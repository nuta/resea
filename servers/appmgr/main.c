#include <list.h>
#include <message.h>
#include <std/malloc.h>
#include <std/printf.h>
#include <std/syscall.h>
#include <std/lookup.h>
#include <string.h>
#include "elf.h"

static task_t init_server;

extern char __zeroed_pages[];
extern char __free_vaddr[];
extern char __free_vaddr_end[];

/// The maximum size of bss + stack + heap.
#define ZEROED_PAGES_SIZE (64 * 1024 * 1024)
#define TID_BASE 16 // FIXME:

/// Task Control Block (TCB).
struct task {
    bool in_use;
    task_t tid;
    char name[32];
    task_t fs_server;
    handle_t handle;
    void *file_header;
    struct elf64_ehdr *ehdr;
    struct elf64_phdr *phdrs;
    bool exited;
    task_t waiter;
};

static struct task tasks[TASKS_MAX];

/// Look for the task in the our task table.
static struct task *get_task_by_tid(task_t tid) {
    if (tid <= 0 || tid > TASKS_MAX) {
        PANIC("invalid tid %d", tid);
    }

    struct task *task = &tasks[tid - TID_BASE];
    ASSERT(task->in_use);
    return task;
}

static error_t read_file(task_t fs_server, handle_t handle, offset_t off,
                         void *buf, size_t len) {
    struct message m;
    m.type = FS_READ_MSG;
    m.fs_read.handle = handle;
    m.fs_read.offset = off;
    m.fs_read.len = len;
    error_t err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        return err;
    }

    ASSERT(m.type == FS_READ_REPLY_MSG);
    ASSERT(len == m.fs_read_reply.len);
    memcpy(buf, m.fs_read_reply.data, m.fs_read_reply.len);
    free(m.fs_read_reply.data);
    return OK;
}

static task_t exec(const char *name, task_t fs_server, handle_t handle) {
    INFO("launching %s...", name);

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
    read_file(fs_server, handle, 0, file_header, PAGE_SIZE);

    // Ensure that it's an ELF file.
    struct elf64_ehdr *ehdr = (struct elf64_ehdr *) file_header;
    if (memcmp(ehdr->e_ident, "\x7f" "ELF", 4) != 0) {
        WARN("%s: invalid ELF magic, ignoring...", name);
        return ERR_NOT_ACCEPTABLE;
    }

    // Create a new task for the server.
    error_t err =
        task_create(task->tid, name, ehdr->e_entry, task_self(), CAP_ALL);
    ASSERT_OK(err);

    task->in_use = true;
    task->fs_server = fs_server;
    task->handle = handle;
    task->file_header = file_header;
    task->ehdr = ehdr;
    task->phdrs = (struct elf64_phdr *) ((uintptr_t) ehdr + ehdr->e_ehsize);
    task->exited = false;
    task->waiter = 0;
    strncpy(task->name, name, sizeof(task->name));

    return task->tid;
}

static void *alloc_page(paddr_t *paddr) {
    struct message m;
    m.type = ALLOC_PAGES_MSG;
    m.alloc_pages.num_pages = 1;
    m.alloc_pages.paddr = 0;
    error_t err = ipc_call(init_server, &m);
    ASSERT_OK(err);
    *paddr = m.alloc_pages_reply.paddr;
    return (void *) m.alloc_pages_reply.vaddr;
}

static paddr_t pager(struct task *task, vaddr_t vaddr, pagefault_t fault) {
    if (fault & PF_PRESENT) {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        WARN("%s: invalid memory access at %p (perhaps segfault?)", task->name,
             vaddr);
        return 0;
    }

    // Zeroed pages.
    vaddr_t zeroed_pages_start = (vaddr_t) __zeroed_pages;
    vaddr_t zeroed_pages_end = zeroed_pages_start + ZEROED_PAGES_SIZE;
    if (zeroed_pages_start <= vaddr && vaddr < zeroed_pages_end) {
        // The accessed page is zeroed one (.bss section, stack, or heap).
        paddr_t paddr;
        void *p = alloc_page(&paddr);
        memset(p, 0, PAGE_SIZE);
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
    paddr_t paddr;
    void *p = alloc_page(&paddr);
    size_t offset_in_segment = (vaddr - phdr->p_vaddr) + phdr->p_offset;
    error_t err =
        read_file(task->fs_server, task->handle, offset_in_segment, p, PAGE_SIZE);
    if (IS_ERROR(err)) {
        WARN("%s: failed to read a file: %s", task->name, err2str(err));
        return 0;
    }

    return paddr;
}

static void kill(struct task *task) {
    task_destroy(task->tid);
    task->in_use = false;
    free(task->file_header);
}

void main(void) {
    INFO("starting...");

    for (int i = 0; i < TASKS_MAX; i++) {
        tasks[i].in_use = false;
        tasks[i].tid = TID_BASE + i;
    }

    init_server = ipc_lookup("init");
    ASSERT_OK(init_server);

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case EXCEPTION_MSG: {
                struct task *task = get_task_by_tid(m.join.task);
                ASSERT(task);
                WARN("%s exited", task->name);
                task->exited = true;
                if (task->waiter) {
                    m.type = JOIN_REPLY_MSG;
                    ipc_reply(task->waiter, &m);
                }
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
            case EXEC_MSG: {
                task_t task_or_err = exec(m.exec.name, m.exec.server, m.exec.handle);
                if (IS_ERROR(task_or_err)) {
                    ipc_reply_err(m.src, task_or_err);
                    break;
                }

                m.type = EXEC_REPLY_MSG;
                m.exec_reply.task = task_or_err;
                ipc_reply(m.src, &m);
                break;
            }
            case JOIN_MSG: {
                struct task *task = get_task_by_tid(m.join.task);
                if (!task) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                if (task->exited) {
                    m.type = JOIN_REPLY_MSG;
                    ipc_reply(m.src, &m);
                } else {
                    task->waiter = m.src;
                }
                break;
            }
            default:
                WARN("unknown message type (type=%d)", m.type);
        }
    }
}
