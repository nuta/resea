#include <list.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/task.h>
#include <string.h>
#include "elf.h"
#include "bootfs.h"
#include "pages.h"

extern char __bootfs[];
extern char __zeroed_pages[];
extern char __zeroed_pages_end[];
extern char __free_vaddr[];
extern char __free_vaddr_end[];

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
    vaddr_t bulk_buf;
    size_t bulk_len;
    task_t received_bulk_from;
    vaddr_t received_bulk_buf;
    size_t received_bulk_len;
    list_t bulk_sender_queue;
    list_elem_t bulk_sender_next;
    struct message bulk_sender_m;
};

static struct task tasks[CONFIG_NUM_TASKS];
static struct bootfs_file *files;
static unsigned num_files;

static paddr_t alloc_pages(struct task *task, vaddr_t vaddr, size_t num_pages);

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

static void init_task_struct(struct task *task, const char *name,
    struct bootfs_file *file, void *file_header, struct elf64_ehdr *ehdr) {
    task->in_use = true;
    task->file = file;
    task->file_header = file_header;
    task->ehdr = ehdr;
    if (ehdr) {
        task->phdrs = (struct elf64_phdr *) ((uintptr_t) ehdr + ehdr->e_ehsize);
    } else {
        task->phdrs = NULL;
    }

    task->free_vaddr = (vaddr_t) __free_vaddr;
    task->bulk_buf = 0;
    task->bulk_len = 0;
    task->received_bulk_buf = 0;
    task->received_bulk_len = 0;
    task->received_bulk_from = 0;
    list_init(&task->bulk_sender_queue);
    list_nullify(&task->bulk_sender_next);
    strncpy(task->name, name, sizeof(task->name));
    list_init(&task->page_areas);
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
        task_create(task->tid, file->name, ehdr->e_entry, task_self(), TASK_IO);
    ASSERT_OK(err);

    init_task_struct(task, file->name, file, file_header, ehdr);
    return task->tid;
}

static error_t map_page(task_t tid, vaddr_t vaddr, paddr_t paddr,
                        unsigned flags, bool overwrite) {
    flags |= overwrite ? (MAP_DELETE | MAP_UPDATE) : MAP_UPDATE;
    while (true) {
        paddr_t kpage = pages_alloc(1);
        error_t err = task_map(tid, vaddr, paddr, kpage, flags);
        switch (err) {
            case ERR_TRY_AGAIN:
                continue;
            default:
                return err;
        }
    }
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
    vaddr_t zeroed_pages_end = (vaddr_t) __zeroed_pages_end;
    if (zeroed_pages_start <= vaddr && vaddr < zeroed_pages_end) {
        // The accessed page is zeroed one (.bss section, stack, or heap).
        paddr_t paddr = alloc_pages(task, vaddr, 1);
        ASSERT_OK(map_page(INIT_TASK_TID, paddr, paddr, MAP_W, false));
        memset((void *) paddr, 0, PAGE_SIZE);
        return paddr;
    }

    // Look for the associated program header.
    struct elf64_phdr *phdr = NULL;
    if (task->ehdr) {
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

        if (phdr) {
            // Allocate a page and fill it with the file data.
            paddr_t paddr = alloc_pages(task, vaddr, 1);
            ASSERT_OK(map_page(INIT_TASK_TID, paddr, paddr, MAP_W, false));
            size_t offset_in_segment = (vaddr - phdr->p_vaddr) + phdr->p_offset;
            read_file(task->file, offset_in_segment, (void *) paddr, PAGE_SIZE);
            return paddr;
        }
    }

    WARN("invalid memory access (addr=%p), killing %s...", vaddr, task->name);
    return 0;
}

static void kill(struct task *task) {
    task_destroy(task->tid);
    task->in_use = false;
    if (task->file_header) {
        free(task->file_header);
    }
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

static paddr_t alloc_pages(struct task *task, vaddr_t vaddr, size_t num_pages) {
    struct page_area *area = malloc(sizeof(*area));
    area->vaddr = vaddr;
    area->paddr = pages_alloc(num_pages);
    area->num_pages = num_pages;
    list_push_back(&task->page_areas, &area->next);
    return area->paddr;
}

static error_t phy_alloc_pages(struct task *task, vaddr_t *vaddr, paddr_t *paddr,
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

static paddr_t vaddr2paddr(struct task *task, vaddr_t vaddr) {
    LIST_FOR_EACH (area, &task->page_areas, struct page_area, next) {
        if (area->vaddr <= vaddr
            && vaddr < area->vaddr + area->num_pages * PAGE_SIZE) {
            return area->paddr + (vaddr - area->vaddr);
        }
    }

    // The page is not mapped. Try filling it with pager.
    return pager(task, vaddr, PF_USER | PF_WRITE /* FIXME: strip PF_WRITE */);
}

static error_t handle_do_bulkcopy(struct message *m);

static error_t handle_accept_bulkcopy(struct message *m) {
    struct task *task = get_task_by_tid(m->src);
    ASSERT(task);

//    TRACE("accept: %s: %p %d (old=%p)",
//        task->name, m->accept_bulkcopy.addr, m->accept_bulkcopy.len, task->bulk_buf);
    if (task->bulk_buf) {
        return ERR_ALREADY_EXISTS;
    }

    task->bulk_buf = m->accept_bulkcopy.addr;
    task->bulk_len = m->accept_bulkcopy.len;

    struct task *sender = LIST_POP_FRONT(&task->bulk_sender_queue, struct task,
                                         bulk_sender_next);
    if (sender) {
        struct message m;
        memcpy(&m, &sender->bulk_sender_m, sizeof(m));
//        TRACE("%s -> %s: src = %d / %d", task->name, sender->name,
//              sender->bulk_sender_m.src, m.src);
        error_t err = handle_do_bulkcopy(&m);
        switch (err) {
            case OK:
                ipc_reply(sender->tid, &m);
                break;
            case DONT_REPLY:
                // Do nothing.
                break;
            default:
                OOPS_OK(err);
                ipc_reply_err(sender->tid, err);
        }
    }

    m->type = ACCEPT_BULKCOPY_REPLY_MSG;
    return OK;
}

static error_t handle_verify_bulkcopy(struct message *m) {
    struct task *task = get_task_by_tid(m->src);
    ASSERT(task);

//    TRACE("verify: %s: id=%p len=%d (src=%d)", task->name,
//          m->verify_bulkcopy.id, m->verify_bulkcopy.len, m->src);
    if (m->verify_bulkcopy.src != task->received_bulk_from
        || m->verify_bulkcopy.id != task->received_bulk_buf
        || m->verify_bulkcopy.len != task->received_bulk_len) {
        return ERR_INVALID_ARG;
    }

    m->type = VERIFY_BULKCOPY_REPLY_MSG;
    m->verify_bulkcopy_reply.received_at = task->received_bulk_buf;

    task->received_bulk_buf = 0;
    task->received_bulk_len = 0;
    task->received_bulk_from = 0;
    return OK;
}

uint8_t __src_page[PAGE_SIZE] __aligned(PAGE_SIZE);
uint8_t __dst_page[PAGE_SIZE] __aligned(PAGE_SIZE);

static error_t handle_do_bulkcopy(struct message *m) {
    struct task *src_task = get_task_by_tid(m->src);
    ASSERT(src_task);

    struct task *dst_task = get_task_by_tid(m->do_bulkcopy.dst);
    if (!dst_task) {
        return ERR_NOT_FOUND;
    }

//    TRACE("do_copy: %s -> %s: %p -> %p, len=%d",
//        src_task->name, dst_task->name,
//        m->do_bulkcopy.addr, dst_task->bulk_buf,
//        m->do_bulkcopy.len);
    if (!dst_task->bulk_buf) {
        // TODO: block the sender until it gets filled.
        memcpy(&src_task->bulk_sender_m, m, sizeof(*m));
        list_push_back(&dst_task->bulk_sender_queue, &src_task->bulk_sender_next);
        return DONT_REPLY;
    }

    size_t len = m->do_bulkcopy.len;
    vaddr_t src_buf = m->do_bulkcopy.addr;
    vaddr_t dst_buf = dst_task->bulk_buf;
    DEBUG_ASSERT(len <= dst_task->bulk_len);

    size_t remaining = len;
    while (remaining > 0) {
        offset_t src_off = src_buf % PAGE_SIZE;
        offset_t dst_off = dst_buf % PAGE_SIZE;
        size_t copy_len = MIN(remaining, MIN(PAGE_SIZE - src_off, PAGE_SIZE - dst_off));

        void *src_ptr;
        if (src_task->tid == INIT_TASK_TID) {
            src_ptr = (void *) src_buf;
        } else {
            paddr_t src_paddr = vaddr2paddr(src_task, ALIGN_DOWN(src_buf, PAGE_SIZE));
            if (!src_paddr) {
                kill(src_task);
                return DONT_REPLY;
            }

            ASSERT_OK(map_page(INIT_TASK_TID, (vaddr_t) __src_page, src_paddr,
                               MAP_W, true));
            src_ptr = &__src_page[src_off];
        }

        void *dst_ptr;
        if (dst_task->tid == INIT_TASK_TID) {
            dst_ptr = (void *) dst_buf;
        } else {
            paddr_t dst_paddr = vaddr2paddr(dst_task, ALIGN_DOWN(dst_buf, PAGE_SIZE));
            if (!dst_paddr) {
                kill(dst_task);
                return ERR_UNAVAILABLE;
            }

            // Temporarily map the pages into the our address space.
            ASSERT_OK(map_page(INIT_TASK_TID, (vaddr_t) __dst_page, dst_paddr,
                               MAP_W, true));
            dst_ptr = &__dst_page[dst_off];
        }

        // Copy between the tasks.
        memcpy(dst_ptr, src_ptr, copy_len);
        remaining -= copy_len;
        dst_buf += copy_len;
        src_buf += copy_len;
    }

    dst_task->received_bulk_buf = dst_task->bulk_buf;
    dst_task->received_bulk_len = m->do_bulkcopy.len;
    dst_task->received_bulk_from = src_task->tid;
    dst_task->bulk_buf = 0;
    dst_task->bulk_len = 0;

    m->type = DO_BULKCOPY_REPLY_MSG;
    m->do_bulkcopy_reply.id = dst_task->received_bulk_buf;
    return OK;
}

error_t call_self(struct message *m) {
    m->src = INIT_TASK_TID;
    error_t err;
    switch (m->type) {
        case ACCEPT_BULKCOPY_MSG:
            err = handle_accept_bulkcopy(m);
            break;
        case VERIFY_BULKCOPY_MSG:
            err = handle_verify_bulkcopy(m);
            break;
        case DO_BULKCOPY_MSG:
            err = handle_do_bulkcopy(m);
            break;
        default:
            UNREACHABLE();
    }

    if (err != OK) {
        PANIC("call_self failed (%s)", err2str(err));
    }

    return err;
}

static void handle_message(const struct message *m) {
    struct message r;
    bzero(&r, sizeof(r));

    switch (m->type) {
        case ACCEPT_BULKCOPY_MSG: {
            memcpy(&r, m, sizeof(r));
            error_t err = handle_accept_bulkcopy(&r);
            switch (err) {
                case DONT_REPLY:
                    break;
                case OK:
                    ipc_reply(m->src, &r);
                    break;
                default:
                    ipc_reply_err(m->src, err);
            }
            break;
        }
        case VERIFY_BULKCOPY_MSG: {
            memcpy(&r, m, sizeof(r));
            error_t err = handle_verify_bulkcopy(&r);
            switch (err) {
                case DONT_REPLY:
                    break;
                case OK:
                    ipc_reply(m->src, &r);
                    break;
                default:
                    ipc_reply_err(m->src, err);
            }
            break;
        }
        case DO_BULKCOPY_MSG: {
            memcpy(&r, m, sizeof(r));
            error_t err = handle_do_bulkcopy(&r);
            switch (err) {
                case DONT_REPLY:
                    break;
                case OK:
                    ipc_reply(m->src, &r);
                    break;
                default:
                    ipc_reply_err(m->src, err);
            }
            break;
        }
        case NOP_MSG:
            r.type = NOP_REPLY_MSG;
            r.nop_reply.value = m->nop.value * 7;
            ipc_reply(m->src, &r);
            break;
        case NOP_WITH_BULK_MSG:
            free((void *) m->nop_with_bulk.data);
            r.type = NOP_WITH_BULK_REPLY_MSG;
            r.nop_with_bulk_reply.data = "reply!";
            r.nop_with_bulk_reply.data_len = 7;
            ipc_reply(m->src, &r);
            break;
        case EXCEPTION_MSG: {
            if (m->src != KERNEL_TASK_TID) {
                WARN("forged exception message from #%d, ignoring...",
                     m->src);
                break;
            }

            struct task *task = get_task_by_tid(m->exception.task);
            ASSERT(task);
            ASSERT(m->exception.task == task->tid);

            if (m->exception.exception == EXP_GRACE_EXIT) {
                INFO("%s: terminated its execution", task->name);
            } else {
                WARN("%s: exception occurred, killing the task...",
                     task->name);
            }

            kill(task);
            break;
        }
        case PAGE_FAULT_MSG: {
            if (m->src != KERNEL_TASK_TID) {
                WARN("forged page fault message from #%d, ignoring...",
                     m->src);
                break;
            }

            struct task *task = get_task_by_tid(m->page_fault.task);
            ASSERT(task);
            ASSERT(m->page_fault.task == task->tid);

            paddr_t paddr =
                pager(task, m->page_fault.vaddr, m->page_fault.fault);
            if (!paddr) {
                ipc_reply_err(m->src, ERR_NOT_FOUND);
                break;
            }

            vaddr_t aligned_vaddr = ALIGN_DOWN(m->page_fault.vaddr, PAGE_SIZE);
            ASSERT_OK(map_page(task->tid, aligned_vaddr, paddr, MAP_W, false));
            r.type = PAGE_FAULT_REPLY_MSG;

            ipc_reply(task->tid, &r);
            break;
        }
        case LOOKUP_MSG: {
            char *name = (char *) m->lookup.name;
            struct task *task = NULL;
            for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
                if (tasks[i].in_use && !strcmp(tasks[i].name, name)) {
                    task = &tasks[i];
                    break;
                }
            }

            free(name);
            if (!task) {
                WARN("Failed to locate the task named '%s'", name);
                ipc_reply_err(m->src, ERR_NOT_FOUND);
                break;
            }

            r.type = LOOKUP_REPLY_MSG;
            r.lookup_reply.task = task->tid;
            ipc_reply(m->src, &r);
            break;
        }
        case ALLOC_PAGES_MSG: {
            struct task *task = get_task_by_tid(m->src);
            ASSERT(task);

            vaddr_t vaddr;
            paddr_t paddr = m->alloc_pages.paddr;
            error_t err =
                phy_alloc_pages(task, &vaddr, &paddr, m->alloc_pages.num_pages);
            if (err != OK) {
                ipc_reply_err(m->src, err);
                break;
            }

            r.type = ALLOC_PAGES_REPLY_MSG;
            r.alloc_pages_reply.vaddr = vaddr;
            r.alloc_pages_reply.paddr = paddr;
            ipc_reply(m->src, &r);
            break;
        }
        case LAUNCH_TASK_MSG: {
            // Look for the program in the apps directory.
            char *name = (char *) m->launch_task.name;
            struct bootfs_file *file = NULL;
            for (uint32_t i = 0; i < num_files; i++) {
                if (!strcmp(files[i].name, name)) {
                    file = &files[i];
                    break;
                }
            }

            free(name);
            if (!file) {
                ipc_reply_err(m->src, ERR_NOT_FOUND);
                break;
            }

            launch_task(file);
            r.type = LAUNCH_TASK_REPLY_MSG;
            ipc_reply(m->src, &r);
            break;
        }
        default:
            WARN("unknown message type (type=%d)", m->type);
            // FIXME: Free bulk payloads.
    }
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

    // Initialize a task struct for bootstrap.
    init_task_struct(&tasks[INIT_TASK_TID - 1], "bootstrap", NULL, NULL, NULL);

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
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);
        handle_message(&m);
    }
}
