#include <list.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/task.h>
#include <cstring.h>
#include "elf.h"
#include "bootfs.h"
#include "pages.h"

extern char __bootfs[];
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
    struct bootfs_file *file;
    void *file_header;
    struct elf64_ehdr *ehdr;
    struct elf64_phdr *phdrs;
    vaddr_t free_vaddr;
    list_t page_areas;
};

static struct task tasks[CONFIG_NUM_TASKS];
static struct bootfs_file *files;
static unsigned num_files;

/// Look for the task in the our task table.
static struct task *get_task_by_tid(task_t tid) {
    if (tid <= 0 || tid > CONFIG_NUM_TASKS) {
        PANIC("invalid tid %d", tid);
    }

    struct task *task = &tasks[tid - 1];
    ASSERT(task->in_use);
    return task;
}

static void read_file(struct bootfs_file *file, offset_t off, void *buf, size_t len) {
    void *p =
        (void *) (((uintptr_t) __bootfs) + file->offset + off);
    memcpy(buf, p, len);
}

static task_t launch_task(struct bootfs_file *file) {
    TRACE("launching %s...", file->name);

    // Look for an unused task ID.
    struct task *task = NULL;
    for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
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
    vaddr = ALIGN_DOWN(vaddr, PAGE_SIZE);

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
    TRACE("starting...");
    struct bootfs_header *header = (struct bootfs_header *) __bootfs;
    num_files = header->num_files;
    files =
        (struct bootfs_file *) (((uintptr_t) &__bootfs) + header->files_off);
    pages_init();

    for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
        tasks[i].in_use = false;
        tasks[i].tid = i + 1;
    }

    // Mark init as in use.
    tasks[0].in_use = true;
    strncpy(tasks[0].name, "bootstrap", sizeof(tasks[0].name));

    // Launch servers in bootfs.
    int num_launched = 0;
    for (uint32_t i = 0; i < num_files; i++) {
        struct bootfs_file *file = &files[i];

        // Autostart server names (separated by whitespace).
        char *startups = AUTOSTARTS;

        // Execute the file if it is listed in the autostarts.
        while (*startups != '\0') {
            size_t len = strlen(file->name);
            if (!strncmp(file->name, startups, len)
                && (startups[len] == '\0' || startups[len] == ' ')) {
                launch_task(file);
                num_launched++;
                break;
            }

            while (*startups != '\0' && *startups != ' ') {
                startups++;
            }

            if (*startups == ' ') {
                startups++;
            }
        }
    }

    if (!num_launched) {
        WARN("no servers to launch");
    }

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        // TODO: bzero(m)
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOP_MSG:
                m.type = NOP_REPLY_MSG;
                m.nop_reply.value = m.nop.value * 7;
                ipc_send(m.src, &m);
                break;
            case NOP_WITH_BULK_MSG:
                free((void *) m.nop_with_bulk.data);
                m.type = NOP_WITH_BULK_REPLY_MSG;
                m.nop_with_bulk_reply.data = "reply!";
                m.nop_with_bulk_reply.data_len = 7;
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

                if (m.exception.exception == EXP_GRACE_EXIT) {
                    INFO("%s: terminated its execution", task->name);
                } else {
                    WARN("%s: exception occurred, killing the task...",
                         task->name);
                }

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
                char *name = (char *) m.lookup.name;
                struct task *task = NULL;
                for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
                    if (tasks[i].in_use && !strcmp(tasks[i].name, name)) {
                        task = &tasks[i];
                        break;
                    }
                }

                if (!task) {
                    WARN("Failed to locate the task named '%s'", name);
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    free(name);
                    break;
                }

                m.type = LOOKUP_REPLY_MSG;
                m.lookup_reply.task = task->tid;
                ipc_reply(m.src, &m);
                free(name);
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
            case LAUNCH_TASK_MSG: {
                // Look for the program in the apps directory.
                char *name = (char *) m.launch_task.name;
                struct bootfs_file *file = NULL;
                for (uint32_t i = 0; i < num_files; i++) {
                    if (!strcmp(files[i].name, name)) {
                        file = &files[i];
                        break;
                    }
                }

                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    free(name);
                    break;
                }

                launch_task(file);
                m.type = LAUNCH_TASK_REPLY_MSG;
                ipc_reply(m.src, &m);
                free(name);
                break;
            }
            default:
                WARN("unknown message type (type=%d)", m.type);
        }
    }
}
