#include "alloc_pages.h"
#include "elf.h"
#include "initfs.h"
#include "process.h"

#include <resea.h>
#include <resea/ipc.h>
#include <resea/string.h>
#include <resea_idl.h>

static cid_t kernel_ch = 1;

extern char __initfs;
error_t handle_fill_page_request(
    struct fill_page_request_msg *m, struct fill_page_request_reply_msg *r) {
    uintptr_t alloced_addr;
    if ((alloced_addr = alloc_pages(1)) == 0)
        return ERR_NO_MEMORY;

    struct process *proc = get_process_by_pid(m->pid);
    assert(proc);

    TRACE("page fault addr: %p", m->addr);
    for (int i = 0; i < proc->elf.num_phdrs; i++) {
        struct elf64_phdr *phdr = &proc->elf.phdrs[i];
        if (phdr->p_vaddr <= m->addr &&
            m->addr < phdr->p_vaddr + phdr->p_memsz) {
            size_t offset = m->addr - phdr->p_vaddr;
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
            TRACE("Filling from initfs fault=%p, off_in_fs=%p, %d", m->addr,
                (uintptr_t) &proc->file->content[fileoff] -
                    (uintptr_t) &__initfs,
                copy_len);
            break;
        }
    }

    r->header = FILL_PAGE_REQUEST_REPLY_HEADER;
    r->page = PAGE_PAYLOAD(alloced_addr, 0, PAGE_TYPE_SHARED);
    return OK;
}

static error_t handle_printchar(
    struct printchar_msg *m, UNUSED struct printchar_reply_msg *r) {
    // Forward to the kernel sever.
    return printchar(kernel_ch, m->ch);
}

static error_t handle_exit_kernel_test(UNUSED struct exit_kernel_test_msg *m,
    UNUSED struct exit_kernel_test_reply_msg *r) {
    // Forward to the kernel sever.
    return exit_kernel_test(kernel_ch);
}

static error_t handle_benchmark_nop(UNUSED struct benchmark_nop_msg *m,
    UNUSED struct benchmark_nop_reply_msg *r) {
    return OK;
}

void mainloop(cid_t server_ch) {
    struct message m;
    TRY_OR_PANIC(ipc_recv(server_ch, &m));
    while (1) {
        struct message r;
        cid_t from = m.from;
        error_t err;
        switch (MSG_TYPE(m.header)) {
        case EXIT_CURRENT_MSG:
            UNIMPLEMENTED();
        case PRINTCHAR_MSG:
            err = dispatch_printchar(handle_printchar, &m, &r);
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
        default:
            WARN("invalid message type %x", MSG_TYPE(m.header));
            err = ERR_INVALID_MESSAGE;
            r.header = ERROR_TO_HEADER(err);
        }

        if (err == ERR_DONT_REPLY) {
            TRY_OR_PANIC(ipc_recv(from, &m));
        } else {
            TRY_OR_PANIC(ipc_replyrecv(from, &r, &m));
        }
    }
}

void main(void) {
    INFO("starting...");
    initfs_init();
    process_init();

    cid_t server_ch;
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

    INFO("enterinng main loop ");
    mainloop(server_ch);
}
