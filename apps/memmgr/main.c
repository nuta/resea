#include "alloc_pages.h"
#include "elf.h"
#include "initfs.h"
#include "process.h"
#include "init_args.h"
#include <resea.h>
#include <resea/ipc.h>
#include <resea/string.h>
#include <resea/math.h>
#include <resea_idl.h>

static cid_t kernel_ch = 1;
extern struct init_args __init_args;

extern char __initfs;
error_t handle_fill_page_request(UNUSED cid_t from, pid_t pid, uintptr_t addr,
                                 UNUSED size_t size, page_t *page) {
    uintptr_t alloced_addr;
    if ((alloced_addr = do_alloc_pages(0)) == 0)
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

    *page = PAGE_PAYLOAD(alloced_addr, 0, PAGE_TYPE_SHARED);
    return OK;
}

static error_t handle_printchar(UNUSED cid_t from, uint8_t ch) {
    // Forward to the kernel sever.
    return printchar(kernel_ch, ch);
}

static error_t handle_exit_kernel_test(UNUSED cid_t from) {
    // Forward to the kernel sever.
    return exit_kernel_test(kernel_ch);
}

static error_t handle_benchmark_nop(UNUSED cid_t from) {
    return OK;
}

static error_t handle_alloc_pages(UNUSED cid_t from, size_t order,
                                  page_t *page) {
    paddr_t paddr = do_alloc_pages(order);
    if (!paddr) {
        return ERR_NO_MEMORY;
    }

    *page = PAGE_PAYLOAD(paddr, order, PAGE_TYPE_SHARED);
    return OK;
}

static error_t handle_get_framebuffer(UNUSED cid_t from, page_t *framebuffer,
                                      int32_t *width, int32_t *height,
                                      uint8_t *bpp) {
    struct framebuffer_info *info = &__init_args.framebuffer;
    paddr_t paddr = info->paddr;
    int order = ulllog2(
        (info->height * info->width * (info->bpp / 8)) / PAGE_SIZE);
    *framebuffer = PAGE_PAYLOAD(paddr, order, PAGE_TYPE_SHARED);
    *width  = (int) info->width;
    *height = (int) info->height;
    *bpp    = info->bpp;
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

static error_t handle_register_server(UNUSED cid_t from, uint8_t interface,
                                      cid_t ch) {
    // TODO: Support multiple servers with the same interface ID.
    assert(servers[interface] == 0);
    TRACE("register_server: interface=%d", interface);

    servers[interface] = ch;
    TRY_OR_PANIC(transfer(servers[interface], server_ch));
    return OK;
}

static error_t handle_connect_server(UNUSED cid_t from, uint8_t interface,
                                     UNUSED cid_t *ch) {
    TRACE("connect_server(interface=%d)", interface);
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

    waiter->reply_to = from;
    waiter->interface = interface;
    waiter->sent_request = false;
    return ERR_DONT_REPLY;
}

static error_t handle_connect_request_reply(struct connect_request_reply_msg *m) {
    struct waiter *waiter = NULL;
    for (int i = 0; i < WAITERS_MAX; i++) {
        if (waiters[i].reply_to && waiters[i].server_ch == m->from) {
            waiter = &waiters[i];
            break;
        }
    }

    assert(waiter);
    send_connect_server_reply(waiter->reply_to, m->ch);
    waiter->reply_to = 0;
    return ERR_DONT_REPLY;
}

static void post_work() {
    for (int interface = 0; interface <= INTERFACE_ID_MAX; interface++) {
        if (!servers[interface]) {
            continue;
        }

        for (int i = 0; i < WAITERS_MAX; i++) {
            if (waiters[i].reply_to && waiters[i].interface == interface
                && !waiters[i].sent_request) {
                TRACE("sending discovery.connect_request(to=%d, interface=%d)",
                    servers[interface], interface);
                send_connect_request(servers[interface], interface);
                TRACE("sent connect_request");
                waiters[i].server_ch = servers[interface];
                waiters[i].sent_request = true;
            }
        }
    }
}

void mainloop(cid_t server_ch) {
    while (1) {
        struct message m;
        TRY_OR_PANIC(ipc_recv(server_ch, &m));

        struct message r;
        error_t err;
        switch (MSG_TYPE(m.header)) {
        case EXIT_CURRENT_MSG:
            UNIMPLEMENTED();
        case PRINTCHAR_MSG:
            err = dispatch_printchar(handle_printchar, &m, &r);
            break;

        //
        //  Discovery
        //
        case REGISTER_SERVER_MSG:
            err = dispatch_register_server(handle_register_server, &m, &r);
            break;
        case CONNECT_SERVER_MSG:
            err = dispatch_connect_server(handle_connect_server, &m, &r);
            break;
        case CONNECT_REQUEST_REPLY_MSG:
            err = handle_connect_request_reply((struct connect_request_reply_msg *) &m);
            break;

        case EXIT_KERNEL_TEST_MSG:
            err = dispatch_exit_kernel_test(handle_exit_kernel_test, &m, &r);
            break;
        case FILL_PAGE_REQUEST_MSG:
            err = dispatch_fill_page_request(handle_fill_page_request, &m, &r);
            break;
        case BENCHMARK_NOP_MSG:
            err = dispatch_benchmark_nop(handle_benchmark_nop, &m, &r);
            break;
        case GET_FRAMEBUFFER_MSG:
            err = dispatch_get_framebuffer(handle_get_framebuffer, &m, &r);
            break;
        case ALLOC_PAGES_MSG:
            err = dispatch_alloc_pages(handle_alloc_pages, &m, &r);
            break;
        default:
            WARN("invalid message type %x", MSG_TYPE(m.header));
            err = ERR_INVALID_MESSAGE;
            r.header = ERROR_TO_HEADER(err);
        }

        if (err != ERR_DONT_REPLY) {
            TRY_OR_PANIC(ipc_send(m.from, &r));
        }

        post_work();
    }
}

void main(void) {
    INFO("starting...");
    init_alloc_pages((struct memory_map *) &__init_args.memory_maps,
                     __init_args.num_memory_maps);
    initfs_init();
    process_init();
    discovery_init();

    TRY_OR_PANIC(open(&server_ch));

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

    INFO("enterinng main loop");
    mainloop(server_ch);
}
