#include <list.h>
#include <message.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/syscall.h>
#include <cstring.h>
#include "elf.h"
#include "initfs.h"
#include "pages.h"

extern struct initfs_header __initfs;
extern char __zeroed_pages[];
extern char __free_vaddr[];
extern char __free_vaddr_end[];

/// The maximum size of bss + stack + heap.
#define ZEROED_PAGES_SIZE (64 * 1024 * 1024)

struct page_area {
    list_elem_t next;
    vaddr_t vaddr;
    paddr_t paddr;
    size_t num_pages;
};

/// Task Control Block (TCB).
struct task {
    bool in_use;
    task_t tid;
    char name[32];
    struct initfs_file *file;
    void *file_header;
    struct elf64_ehdr *ehdr;
    struct elf64_phdr *phdrs;
    vaddr_t free_vaddr;
    list_t page_areas;
};

static struct task tasks[TASKS_MAX];

/// Look for the task in the our task table.
static struct task *get_task_by_tid(task_t tid) {
    if (tid <= 0 || tid > TASKS_MAX) {
        PANIC("invalid tid %d", tid);
    }

    struct task *task = &tasks[tid - 1];
    ASSERT(task->in_use);
    return task;
}

static void read_file(struct initfs_file *file, offset_t off, void *buf, size_t len) {
    void *p =
        (void *) (((uintptr_t) &__initfs) + file->offset + off);
    memcpy(buf, p, len);
}

static task_t launch_server(struct initfs_file *file) {
    INFO("launching %s...", file->name);

    // Look for an unused task ID.
    struct task *task = NULL;
    for (int i = 0; i < TASKS_MAX; i++) {
        if (!tasks[i].in_use) {
            task = &tasks[i];
            break;
        }
    }

    if (!task) {
        PANIC("too many tasks");
    }

    void *file_header = malloc(PAGE_SIZE);
    read_file(file, 0, file_header, PAGE_SIZE);

    // Ensure that it's an ELF file.
    struct elf64_ehdr *ehdr = (struct elf64_ehdr *) file_header;
    if (memcmp(ehdr->e_ident, "\x7f" "ELF", 4) != 0) {
        WARN("%s: invalid ELF magic, ignoring...", file->name);
        return ERR_NOT_ACCEPTABLE;
    }

    // Create a new task for the server.
    error_t err =
        task_create(task->tid, file->name, ehdr->e_entry, task_self(), CAP_ALL);
    ASSERT_OK(err);

    task->in_use = true;
    task->file = file;
    task->file_header = file_header;
    task->ehdr = ehdr;
    task->phdrs = (struct elf64_phdr *) ((uintptr_t) ehdr + ehdr->e_ehsize);
    task->free_vaddr = (vaddr_t) __free_vaddr;
    strncpy(task->name, file->name, sizeof(task->name));
    list_init(&task->page_areas);
    return task->tid;
}

static paddr_t pager(struct task *task, vaddr_t vaddr, pagefault_t fault) {
    if (fault & PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        WARN("%s: invalid memory access at %p (perhaps segfault?)", task->name,
             vaddr);
        return 0;
    }

    LIST_FOR_EACH (area, &task->page_areas, struct page_area, next) {
        if (area->vaddr <= vaddr
            && vaddr < area->vaddr + area->num_pages * PAGE_SIZE) {
            return area->paddr + (vaddr - area->vaddr);
        }
    }

    // Zeroed pages.
    vaddr_t zeroed_pages_start = (vaddr_t) __zeroed_pages;
    vaddr_t zeroed_pages_end = zeroed_pages_start + ZEROED_PAGES_SIZE;
    if (zeroed_pages_start <= vaddr && vaddr < zeroed_pages_end) {
        // The accessed page is zeroed one (.bss section, stack, or heap).
        paddr_t paddr = pages_alloc(1);
        memset((void *) paddr, 0, PAGE_SIZE);
        return paddr;
    }

    // Look for the associated program header.
    struct elf64_phdr *phdr = NULL;
    for (unsigned i = 0; i < task->ehdr->e_phnum; i++) {
        // Ignore GNU_STACK
        if (!task->phdrs[i].p_vaddr) {
            continue;
        }

        vaddr_t start = task->phdrs[i].p_vaddr;
        vaddr_t end = start + task->phdrs[i].p_memsz;
        if (start <= vaddr && vaddr <= end) {
            phdr = &task->phdrs[i];
            break;
        }
    }

    if (!phdr) {
        WARN("invalid memory access (addr=%p), killing %s...", vaddr, task->name);
        return 0;
    }
    // Allocate a page and fill it with the file data.
    paddr_t paddr = pages_alloc(1);
    size_t offset_in_segment = (vaddr - phdr->p_vaddr) + phdr->p_offset;
    read_file(task->file, offset_in_segment, (void *) paddr, PAGE_SIZE);
    return paddr;
}

static void kill(struct task *task) {
    task_destroy(task->tid);
    task->in_use = false;
    free(task->file_header);
}

/// Allocates a virtual address space by so-called the bump pointer allocation
/// algorithm.
static vaddr_t alloc_virt_pages(struct task *task, size_t num_pages) {
    vaddr_t vaddr = task->free_vaddr;
    size_t size = num_pages * PAGE_SIZE;

    if (vaddr + size >= (vaddr_t) __free_vaddr_end) {
        // Task's virtual memory space has been exhausted.
        kill(task);
        return 0;
    }

    task->free_vaddr += size;
    return vaddr;
}

static error_t alloc_pages(struct task *task, vaddr_t *vaddr, paddr_t *paddr,
                           size_t num_pages) {
    if (*paddr && !is_mappable_paddr(*paddr)) {
        return ERR_INVALID_ARG;
    }

    *vaddr = alloc_virt_pages(task, num_pages);
    if (*paddr) {
        pages_incref(paddr2pfn(*paddr), num_pages);
    } else {
        *paddr = pages_alloc(num_pages);
    }

    struct page_area *area = malloc(sizeof(*area));
    area->vaddr = *vaddr;
    area->paddr = *paddr;
    area->num_pages = num_pages;
    list_push_back(&task->page_areas, &area->next);
    return OK;
}

void main(void) {
    INFO("starting...");
    pages_init();

    for (int i = 0; i < TASKS_MAX; i++) {
        tasks[i].in_use = false;
        tasks[i].tid = i + 1;
    }

    // Mark init as in use.
    tasks[0].in_use = true;
    strncpy(tasks[0].name, "init", sizeof(tasks[0].name));

    // Launch servers in initfs.
    struct initfs_file *files =
        (struct initfs_file *) (((uintptr_t) &__initfs) + __initfs.files_off);
    for (uint32_t i = 0; i < __initfs.num_files; i++) {
        launch_server(&files[i]);
    }

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct ipc_msg_t m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOP_MSG:
                m.type = NOP_MSG;
                ipc_send(m.src, &m);
                break;
            case EXCEPTION_MSG: {
                if (m.src != KERNEL_TASK_TID) {
                    WARN("forged exception message from #%d, ignoring...",
                         m.src);
                    break;
                }

                struct task *task = get_task_by_tid(m.exception.task);
                ASSERT(task);
                ASSERT(m.exception.task == task->tid);

                WARN("%s: exception occurred, killing the task...", task->name);
                kill(task);
                break;
            }
            case PAGE_FAULT_MSG: {
                if (m.src != KERNEL_TASK_TID) {
                    WARN("forged page fault message from #%d, ignoring...",
                         m.src);
                    break;
                }

                struct task *task = get_task_by_tid(m.page_fault.task);
                ASSERT(task);
                ASSERT(m.page_fault.task == task->tid);

                paddr_t paddr =
                    pager(task, m.page_fault.vaddr, m.page_fault.fault);
                if (paddr) {
                    m.type = PAGE_FAULT_REPLY_MSG;
                    m.page_fault_reply.paddr = paddr;
                    m.page_fault_reply.attrs = PAGE_WRITABLE;
                    error_t err = ipc_send_noblock(task->tid, &m);
                    ASSERT_OK(err);
                } else {
                    kill(task);
                }
                break;
            }
            case LOOKUP_MSG: {
                struct task *task = NULL;
                for (int i = 0; i < TASKS_MAX; i++) {
                    if (tasks[i].in_use && !strcmp(tasks[i].name, m.lookup.name)) {
                        task = &tasks[i];
                        break;
                    }
                }

                if (!task) {
                    WARN("error!");
                    ipc_send_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                m.type = LOOKUP_REPLY_MSG;
                m.lookup_reply.task = task->tid;
                ipc_send(m.src, &m);
                break;
            }
            case ALLOC_PAGES_MSG: {
                struct task *task = get_task_by_tid(m.src);
                ASSERT(task);

                vaddr_t vaddr;
                paddr_t paddr = m.alloc_pages.paddr;
                error_t err =
                    alloc_pages(task, &vaddr, &paddr, m.alloc_pages.num_pages);
                if (err != OK) {
                    ipc_send_err(m.src, err);
                    break;
                }

                m.type = ALLOC_PAGES_REPLY_MSG;
                m.alloc_pages_reply.vaddr = vaddr;
                m.alloc_pages_reply.paddr = paddr;
                ipc_send(m.src, &m);
                break;
            }
            default:
                WARN("unknown message type (type=%d)", m.type);
        }
    }
}
