#include <resea.h>
#include <idl_stubs.h>
#include <server.h>
#include "process.h"
#include "fs.h"

static cid_t memmgr_ch = 1;
static cid_t kernel_ch = 2;
static cid_t server_ch;

static error_t handle_pager_fill(struct message *m) {
    pid_t pid      = m->payloads.pager.fill.pid;
    uintptr_t addr = m->payloads.pager.fill.addr;

    struct process *proc = get_process_by_pid(pid);
    assert(proc);

    // TRACE("page fault addr: %p", addr);
    for (int i = 0; i < proc->elf.num_phdrs; i++) {
        struct elf64_phdr *phdr = &proc->elf.phdrs[i];
        if (phdr->p_vaddr <= addr && addr < phdr->p_vaddr + phdr->p_memsz) {
            size_t fileoff = phdr->p_offset + (addr - phdr->p_vaddr);

            page_base_t page_base = valloc(4096);
            uint8_t *data;
            size_t num_pages;
            TRY_OR_PANIC(call_fs_read(proc->file->fs_server, proc->file->fd,
                fileoff, PAGE_SIZE, page_base, (uintptr_t *) &data, &num_pages));

            m->header = PAGER_FILL_REPLY_HEADER;
            m->payloads.pager.fill_reply.page = PAGE_PAYLOAD((uintptr_t) data, 0);
            return OK;
        }
    }

    // zero-filled page
    page_base_t page_base = valloc(4096);
    uint8_t *ptr;
    size_t num_pages;
    TRY_OR_PANIC(call_memory_alloc_pages(memmgr_ch, 0, page_base,
                                         (uintptr_t *) &ptr, &num_pages));

    m->header = PAGER_FILL_REPLY_HEADER;
    m->payloads.pager.fill_reply.page = PAGE_PAYLOAD((uintptr_t) ptr, 0);
    return OK;
}

static error_t handle_runtime_printchar(struct message *m) {
    // Forward to the kernel sever.
    uint8_t ch = m->payloads.runtime.printchar.ch;
    return call_runtime_printchar(kernel_ch, ch);
}

#define FILES_MAX 32
static struct file files[FILES_MAX];
static struct file *open_app_file(const char *path) {
    struct file *f = NULL;
    for (int i = 0; i < FILES_MAX; i++) {
        if (!files[i].using) {
            f = &files[i];
            break;
        }
    }

    assert(f);

    fd_t handle;
    if (call_fs_open(memmgr_ch, path, 0, &handle) != OK) {
        return NULL;
    }

    f->fs_server = memmgr_ch;
    f->fd = handle;
    f->using = true;
    return f;
}

static error_t handle_api_create_app(struct message *m) {
    const char *path = m->payloads.api.create_app.path;

    struct file *f = open_app_file(path);
    if (!f) {
        return ERR_INVALID_MESSAGE;
    }

    pid_t pid;
    TRY_OR_PANIC(create_app(kernel_ch, memmgr_ch, server_ch, f, &pid));

    m->header = API_CREATE_APP_REPLY_HEADER;
    m->payloads.api.create_app_reply.pid = pid;
    return OK;
}

static error_t handle_api_start_app(struct message *m) {
    pid_t pid = m->payloads.api.start_app.pid;
    TRY_OR_PANIC(start_app(kernel_ch, pid));
    m->header = API_START_APP_REPLY_HEADER;
    return OK;
}

struct join_request {
    bool using;
    cid_t waiter;
    pid_t target;
};

#define JOINS_MAX 16
static struct join_request joins[JOINS_MAX];

static error_t handle_api_join_app(struct message *m) {
    pid_t pid = m->payloads.api.join_app.pid;

    struct join_request *j = NULL;
    for (int i = 0; i < JOINS_MAX; i++) {
        if (!joins[i].using) {
            j = &joins[i];
            break;
        }
    }

    assert(j);

    j->using = true;
    j->target = pid;
    j->waiter = m->from;
    return ERR_DONT_REPLY;
}

static error_t handle_api_exit_app(struct message *m) {
    int8_t code = m->payloads.api.exit_app.code;

    struct process *proc = get_process_by_cid(m->from);
    assert(proc);

    struct join_request *j = NULL;
    for (int i = 0; i < JOINS_MAX; i++) {
        if (joins[i].using && joins[i].target == proc->pid) {
            j = &joins[i];
            break;
        }
    }

    assert(j);

    send_api_join_app_reply(j->waiter, code);
    j->using = false;
    return ERR_DONT_REPLY;
}

static error_t process_message(struct message *m) {
    switch (MSG_TYPE(m->header)) {
    // TODO:
    // case RUNTIME_EXIT_MSG: return handle_runtime_exit(m);
    case RUNTIME_PRINTCHAR_MSG: return handle_runtime_printchar(m);
    case PAGER_FILL_MSG:        return handle_pager_fill(m);
    case API_CREATE_APP_MSG:    return handle_api_create_app(m);
    case API_START_APP_MSG:     return handle_api_start_app(m);
    case API_JOIN_APP_MSG:      return handle_api_join_app(m);
    case API_EXIT_APP_MSG:      return handle_api_exit_app(m);
    }
    return ERR_UNEXPECTED_MESSAGE;
}

int main(void) {
    INFO("starting...");
    TRY_OR_PANIC(open(&server_ch));

    cid_t gui_ch;
    TRY_OR_PANIC(call_discovery_connect(memmgr_ch, GUI_INTERFACE, &gui_ch));

    process_init();

    for (int i = 0; i < FILES_MAX; i++) {
        files[i].using = false;
    }

    for (int i = 0; i < JOINS_MAX; i++) {
        joins[i].using = false;
    }

    fd_t handle;
    TRY_OR_PANIC(call_fs_open(memmgr_ch, "apps/shell.elf", 0, &handle));

    static struct file shell_file;
    shell_file.fs_server = memmgr_ch;
    shell_file.fd = handle;

    pid_t app_pid;
    TRACE("starting shell app");
    TRY_OR_PANIC(create_app(kernel_ch, memmgr_ch, server_ch, &shell_file, &app_pid));
    TRY_OR_PANIC(start_app(kernel_ch, app_pid));
    TRACE("started shell app");

    INFO("entering the mainloop...");
    server_mainloop(server_ch, process_message);
    return 0;
}
