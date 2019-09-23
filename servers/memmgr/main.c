#include "alloc_pages.h"
#include "elf.h"
#include "initfs.h"
#include "process.h"
#include "init_args.h"
#include <resea.h>
#include <resea/ipc.h>
#include <resea/string.h>
#include <resea/math.h>
#include <server.h>
#include <resea_idl.h>

static cid_t kernel_ch = 1;
extern struct init_args __init_args;
extern char __initfs;

static error_t handle_pager_fill(struct message *m) {
    pid_t pid      = m->payloads.pager.fill.pid;
    uintptr_t addr = m->payloads.pager.fill.addr;

    uintptr_t alloced_addr;
    if ((alloced_addr = alloc_pages(0)) == 0)
        return ERR_NO_MEMORY;

    struct process *proc = get_process_by_pid(pid);
    assert(proc);

    // TRACE("page fault addr: %p", addr);
    for (int i = 0; i < proc->elf.num_phdrs; i++) {
        struct elf64_phdr *phdr = &proc->elf.phdrs[i];
        if (phdr->p_vaddr <= addr &&
            addr < phdr->p_vaddr + phdr->p_memsz) {
            size_t offset = addr - phdr->p_vaddr;
            size_t fileoff = phdr->p_offset + offset;
            if (fileoff >= proc->file->len) {
                // Invalid file offset (perhaps p_offset is invalid). Just
                // ignore the segment for now.
                continue;
            }

            size_t copy_len = MIN(MIN(PAGE_SIZE, proc->file->len - fileoff),
                                  phdr->p_filesz - offset);
            memcpy_s((void *) alloced_addr, 4096, &proc->file->content[fileoff],
                copy_len);
            // TRACE("Filling from initfs fault=%p, off_in_fs=%p, %d", addr,
            //     (uintptr_t) &proc->file->content[fileoff] -
            //         (uintptr_t) &__initfs,
            //     copy_len);
            break;
        }
    }

    m->header = PAGER_FILL_REPLY_HEADER;
    m->payloads.pager.fill_reply.page = PAGE_PAYLOAD(alloced_addr, 0);
    return OK;
}

static error_t handle_runtime_exit_current(UNUSED struct message *m) {
    UNIMPLEMENTED();
    return ERR_DONT_REPLY;
}

static error_t handle_runtime_printchar(struct message *m) {
    // Forward to the kernel sever.
    uint8_t ch = m->payloads.runtime.printchar.ch;
    TRY(call_runtime_printchar(kernel_ch, ch));
    m->header = RUNTIME_PRINTCHAR_REPLY_HEADER;
    return OK;
}

static error_t handle_memmgr_benchmark_nop(struct message *m) {
    m->header = MEMMGR_BENCHMARK_NOP_REPLY_HEADER;
    return OK;
}

static error_t handle_memmgr_alloc_pages(struct message *m) {
    size_t order = m->payloads.memmgr.alloc_pages.order;
    paddr_t paddr = alloc_pages(order);
    if (!paddr) {
        return ERR_NO_MEMORY;
    }

    m->header = MEMMGR_ALLOC_PAGES_REPLY_HEADER;
    m->payloads.memmgr.alloc_pages_reply.page = PAGE_PAYLOAD(paddr, order);
    return OK;
}

static error_t handle_memmgr_get_framebuffer(struct message *m) {
    struct framebuffer_info *info = &__init_args.framebuffer;
    paddr_t paddr = info->paddr;
    int order = ulllog2(
        (info->height * info->width * (info->bpp / 8)) / PAGE_SIZE);

    m->header = MEMMGR_GET_FRAMEBUFFER_REPLY_HEADER;
    m->payloads.memmgr.get_framebuffer_reply.framebuffer = PAGE_PAYLOAD(paddr, order);
    m->payloads.memmgr.get_framebuffer_reply.width = (int) info->width;
    m->payloads.memmgr.get_framebuffer_reply.height = (int) info->height;
    m->payloads.memmgr.get_framebuffer_reply.bpp = info->bpp;
    return OK;
}

#define INTERFACE_ID_MAX 0xff
cid_t servers[INTERFACE_ID_MAX + 1];

struct waiter {
    uint8_t interface;
    cid_t reply_to;
    cid_t server_ch;
    bool sent_request;
};
#define WAITERS_MAX 32
static struct waiter waiters[WAITERS_MAX];

static void discovery_init(void) {
    for (int i = 0; i <= INTERFACE_ID_MAX; i++) {
        servers[i] = 0;
    }

    for (int i = 0; i < WAITERS_MAX; i++) {
        waiters[i].reply_to = 0;
    }
}

static cid_t server_ch;

static error_t handle_discovery_publicize(struct message *m) {
    // TODO: Support multiple servers with the same interface ID.
    uint8_t interface = m->payloads.discovery.publicize.interface;
    cid_t ch = m->payloads.discovery.publicize.ch;

    assert(servers[interface] == 0);
    TRACE("register_server: interface=%d", interface);

    servers[interface] = ch;
    TRY_OR_PANIC(transfer(servers[interface], server_ch));

    m->header = DISCOVERY_PUBLICIZE_REPLY_HEADER;
    return OK;
}

static error_t handle_discovery_connect(struct message *m) {
    uint8_t interface = m->payloads.discovery.connect.interface;
    TRACE("discovery_connect(interface=%d)", interface);

    struct waiter *waiter = NULL;
    for (int i = 0; i < WAITERS_MAX; i++) {
        if (waiters[i].reply_to == 0) {
            waiter = &waiters[i];
            break;
        }
    }

    if (!waiter) {
        return ERR_NO_MEMORY;
    }

    waiter->reply_to = m->from;
    waiter->interface = interface;
    waiter->sent_request = false;
    return ERR_DONT_REPLY;
}

static error_t handle_server_connect_reply(struct message *m) {
    cid_t ch = m->payloads.server.connect_reply.ch;

    struct waiter *waiter = NULL;
    for (int i = 0; i < WAITERS_MAX; i++) {
        if (waiters[i].reply_to && waiters[i].server_ch == m->from) {
            waiter = &waiters[i];
            break;
        }
    }

    assert(waiter);
    send_discovery_connect_reply(waiter->reply_to, ch);
    waiter->reply_to = 0;
    return ERR_DONT_REPLY;
}

#define FD_TABLE_MAX 64
struct fd_table_entry {
    const struct initfs_file *file;
};

// TODO: use hash table
// TODO: process-local fd numbers
static struct fd_table_entry fd_table[FD_TABLE_MAX];

static error_t handle_fs_open(struct message *m) {
    const char *path = m->payloads.fs.open.path;

    struct fd_table_entry *entry = NULL;
    fd_t fd;
    for (fd = 0; fd < FD_TABLE_MAX; fd++) {
        if (!fd_table[fd].file) {
            entry = &fd_table[fd];
            break;
        }
    }

    assert(entry);

    struct initfs_dir dir;
    initfs_opendir(&dir);
    const struct initfs_file *file;
    while ((file = initfs_readdir(&dir)) != NULL) {
        if (strcmp(path, file->path) == 0) {
            entry->file = file;
            m->header = FS_OPEN_REPLY_HEADER;
            m->payloads.fs.open_reply.handle = fd;
            return OK;
        }
    }

    WARN("no such a file: '%s'", path);
    return ERR_NOT_FOUND;
}

static error_t handle_fs_read(struct message *m) {
    int32_t handle = m->payloads.fs.read.handle;
    size_t offset  = m->payloads.fs.read.offset;
    size_t len     = m->payloads.fs.read.len;

    assert(handle < FD_TABLE_MAX);
    assert(fd_table[handle].file != NULL);

    const struct initfs_file *file = fd_table[handle].file;

    uintptr_t alloced_addr;
    if ((alloced_addr = alloc_pages(0)) == 0) {
        return ERR_NO_MEMORY;
    }

    assert(len <= PAGE_SIZE);
    assert(offset < file->len);
    size_t copy_len = MIN(len, file->len - offset);
    memcpy_s((void *) alloced_addr, PAGE_SIZE, &file->content[offset],
             copy_len);

    m->header = FS_READ_REPLY_HEADER;
    m->payloads.fs.read_reply.data = PAGE_PAYLOAD(alloced_addr, 0);
    return OK;
}

static error_t deferred_work(void) {
    bool needs_retry = false;
    for (int interface = 0; interface <= INTERFACE_ID_MAX; interface++) {
        if (!servers[interface]) {
            continue;
        }

        for (int i = 0; i < WAITERS_MAX; i++) {
            if (waiters[i].reply_to && waiters[i].interface == interface
                && !waiters[i].sent_request) {
                TRACE("sending discovery.connect_request(to=%d, interface=%d)",
                    servers[interface], interface);

                error_t err;
                err = async_send_server_connect(servers[interface], interface);
                if (err == ERR_WOULD_BLOCK) {
                    TRACE("Would block, ignoring...");
                    needs_retry = true;
                    continue;
                }

                TRY_OR_PANIC(err);
                TRACE("sent connect_request");
                waiters[i].server_ch = servers[interface];
                waiters[i].sent_request = true;
            }
        }
    }

    return needs_retry ? ERR_NEEDS_RETRY : OK;
}

static error_t process_message(struct message *m) {
    switch (MSG_TYPE(m->header)) {
    case NOTIFICATION_MSG: return ERR_DONT_REPLY;
    //
    //  Memmgr
    //
    case MEMMGR_BENCHMARK_NOP_MSG:   return handle_memmgr_benchmark_nop(m);
    case MEMMGR_ALLOC_PAGES_MSG:     return handle_memmgr_alloc_pages(m);
    case MEMMGR_GET_FRAMEBUFFER_MSG: return handle_memmgr_get_framebuffer(m);

    //
    //  Pager for the startup servers.
    //
    case PAGER_FILL_MSG: return handle_pager_fill(m);

    //
    //  Runtime interface for the startup servers.
    //
    case RUNTIME_EXIT_CURRENT_MSG: return handle_runtime_exit_current(m);
    case RUNTIME_PRINTCHAR_MSG: return handle_runtime_printchar(m);

    //
    //  Discovery
    //
    case DISCOVERY_PUBLICIZE_MSG: return handle_discovery_publicize(m);
    case DISCOVERY_CONNECT_MSG: return handle_discovery_connect(m);
    case SERVER_CONNECT_REPLY_MSG: return handle_server_connect_reply(m);

    //
    //  FS server for initfs
    //
    case FS_OPEN_MSG: return handle_fs_open(m);
    // TODO: case FS_CLOSE_MSG: return handle_fs_close(m);
    case FS_READ_MSG: return handle_fs_read(m);
    }

    return ERR_UNEXPECTED_MESSAGE;
}

static void launch_startup_servers(void) {
    struct initfs_dir dir;
    initfs_opendir(&dir);
    const struct initfs_file *file;
    while ((file = initfs_readdir(&dir)) != NULL) {
        if (strncmp(file->path, "startups/", 9) == 0) {
            error_t err = spawn_process(kernel_ch, server_ch, file);
            if (err != OK) {
                ERROR("failed to spawn '%s' (%d)", file->path, err);
            }
        }
    }
}

void main(void) {
    INFO("starting...");

    TRY_OR_PANIC(open(&server_ch));
    init_alloc_pages((struct memory_map *) &__init_args.memory_maps,
                     __init_args.num_memory_maps);
    initfs_init();
    process_init();
    discovery_init();
    launch_startup_servers();

    INFO("enterinng main loop");
    server_mainloop_with_deferred(server_ch, process_message, deferred_work);
}
