#include <list.h>
#include <message.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/syscall.h>
#include <string.h>
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

#define TCBS_MAX 32

/// Task Control Block (TCB).
struct tcb {
    bool in_use;
    tid_t tid;
    const char *name;
    struct initfs_file *file;
    struct elf64_ehdr *ehdr;
    struct elf64_phdr *phdrs;
    vaddr_t free_vaddr;
    list_t page_areas;
};

static struct tcb tcbs[TCBS_MAX];

/// Look for the task in the our task table.
static struct tcb *get_task_by_tid(tid_t tid) {
    if (tid <= 0 || tid > TCBS_MAX) {
        PANIC("invalid tid %d", tid);
    }

    struct tcb *tcb = &tcbs[tid - 1];
    ASSERT(tcb->in_use);
    return tcb;
}

static const void *file_contents(struct initfs_file *file, size_t off) {
    return (void *) (((uintptr_t) &__initfs) + file->offset + off);
}

static void launch_server(struct initfs_file *file) {
    INFO("launching %s...", file->name);

    // Look for an unused task ID.
    struct tcb *tcb = NULL;
    for (int i = 0; i < TCBS_MAX; i++) {
        if (!tcbs[i].in_use) {
            tcb = &tcbs[i];
            break;
        }
    }

    if (!tcb) {
        PANIC("too many tcbs");
    }

    // Ensure that it's an ELF file.
    struct elf64_ehdr *ehdr = (struct elf64_ehdr *) file_contents(file, 0);
    if (memcmp(ehdr->e_ident,
               "\x7f"
               "ELF",
               4)
        != 0) {
        WARN("%s: invalid ELF magic, ignoring...", file->name);
        return;
    }

    // Create a new task for the server.
    error_t err =
        task_create(tcb->tid, file->name, ehdr->e_entry, task_self(), CAP_ALL);
    ASSERT_OK(err);

    tcb->in_use = true;
    tcb->name = file->name;
    tcb->file = file;
    tcb->ehdr = ehdr;
    tcb->phdrs = (struct elf64_phdr *) ((uintptr_t) ehdr + ehdr->e_ehsize);
    tcb->free_vaddr = (vaddr_t) __free_vaddr;
    list_init(&tcb->page_areas);
}

static paddr_t pager(struct tcb *tcb, vaddr_t vaddr, pagefault_t fault) {
    if (fault & PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        WARN("%s: invalid memory access at %p (perhaps segfault?)", tcb->name,
             vaddr);
        return 0;
    }

    LIST_FOR_EACH (area, &tcb->page_areas, struct page_area, next) {
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
    for (unsigned i = 0; i < tcb->ehdr->e_phnum; i++) {
        // Ignore GNU_STACK
        if (!tcb->phdrs[i].p_vaddr) {
            continue;
        }

        vaddr_t start = tcb->phdrs[i].p_vaddr;
        vaddr_t end = start + tcb->phdrs[i].p_memsz;
        if (start <= vaddr && vaddr <= end) {
            phdr = &tcb->phdrs[i];
            break;
        }
    }

    if (!phdr) {
        WARN("invalid memory access (addr=%p), killing %s...", vaddr, tcb->name);
        return 0;
    }
    // Allocate a page and fill it with the file data.
    paddr_t paddr = pages_alloc(1);
    size_t offset_in_segment = (vaddr - phdr->p_vaddr) + phdr->p_offset;
    memcpy((void *) paddr, file_contents(tcb->file, offset_in_segment),
           PAGE_SIZE);

    return paddr;
}

static void kill(struct tcb *tcb) {
    task_destroy(tcb->tid);
    tcb->in_use = false;
}

/// Allocates a virtual address space by so-called the bump pointer allocation
/// algorithm.
static vaddr_t alloc_virt_pages(struct tcb *tcb, size_t num_pages) {
    vaddr_t vaddr = tcb->free_vaddr;
    size_t size = num_pages * PAGE_SIZE;

    if (vaddr + size >= (vaddr_t) __free_vaddr_end) {
        // Task's virtual memory space has been exhausted.
        kill(tcb);
        return 0;
    }

    tcb->free_vaddr += size;
    return vaddr;
}

static error_t alloc_pages(struct tcb *tcb, vaddr_t *vaddr, paddr_t *paddr,
                           size_t num_pages) {
    if (*paddr && !is_mappable_paddr(*paddr)) {
        return ERR_INVALID_ARG;
    }

    *vaddr = alloc_virt_pages(tcb, num_pages);
    if (*paddr) {
        pages_incref(paddr2pfn(*paddr), num_pages);
    } else {
        *paddr = pages_alloc(num_pages);
    }

    struct page_area *area = malloc(sizeof(*area));
    area->vaddr = *vaddr;
    area->paddr = *paddr;
    area->num_pages = num_pages;
    list_push_back(&tcb->page_areas, &area->next);

    INFO("%s: alloc_pages: %p -> %p", tcb->file->name, *vaddr, *paddr);
    return OK;
}

void main(void) {
    INFO("starting...");
    pages_init();

    for (int i = 0; i < TCBS_MAX; i++) {
        tcbs[i].in_use = false;
        tcbs[i].file = NULL;
        tcbs[i].tid = i + 1;
    }

    // Mark init as in use.
    tcbs[0].in_use = true;

    // Launch servers in initfs.
    struct initfs_file *files =
        (struct initfs_file *) (((uintptr_t) &__initfs) + __initfs.files_off);
    for (uint32_t i = 0; i < __initfs.num_files; i++) {
        launch_server(&files[i]);
    }

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
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

                struct tcb *tcb = get_task_by_tid(m.exception.task);
                ASSERT(tcb);
                ASSERT(m.exception.task == tcb->tid);

                WARN("%s: exception occurred, killing the task...", tcb->name);
                kill(tcb);
                break;
            }
            case PAGE_FAULT_MSG: {
                if (m.src != KERNEL_TASK_TID) {
                    WARN("forged page fault message from #%d, ignoring...",
                         m.src);
                    break;
                }

                struct tcb *tcb = get_task_by_tid(m.page_fault.task);
                ASSERT(tcb);
                ASSERT(m.page_fault.task == tcb->tid);

                paddr_t paddr =
                    pager(tcb, m.page_fault.vaddr, m.page_fault.fault);
                if (paddr) {
                    m.type = PAGE_FAULT_REPLY_MSG;
                    m.page_fault_reply.paddr = paddr;
                    m.page_fault_reply.attrs = PAGE_WRITABLE;
                    error_t err = ipc_send_noblock(tcb->tid, &m);
                    ASSERT_OK(err);
                } else {
                    kill(tcb);
                }
                break;
            }
            case LOOKUP_MSG: {
                struct tcb *tcb = NULL;
                for (int i = 0; i < TCBS_MAX; i++) {
                    if (tcbs[i].in_use && tcbs[i].file
                        && !strcmp(tcbs[i].file->name, m.lookup.name)) {
                        tcb = &tcbs[i];
                        break;
                    }
                }

                if (!tcb) {
                    WARN("error!");
                    ipc_send_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                m.type = LOOKUP_REPLY_MSG;
                m.lookup_reply.task = tcb->tid;
                ipc_send(m.src, &m);
                break;
            }
            case ALLOC_PAGES_MSG: {
                struct tcb *tcb = get_task_by_tid(m.src);
                ASSERT(tcb);

                vaddr_t vaddr;
                paddr_t paddr = m.alloc_pages.paddr;
                error_t err =
                    alloc_pages(tcb, &vaddr, &paddr, m.alloc_pages.num_pages);
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
