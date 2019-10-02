#include <resea.h>
#include <idl_stubs.h>
#include <elf.h>
#include "process.h"
#include "fs.h"

// FIXME:
#define APP_IMAGE_START 0x01000000
#define APP_IMAGE_SIZE 0x01000000
#define APP_ZEROED_PAGES_START 0x02000000
#define APP_ZEROED_PAGES_SIZE 0x02000000
#define APP_INITIAL_STACK_POINTER 0x03000000

// TODO: Use hash table.
#define NUM_PROCESSES_MAX 128
static struct process processes[NUM_PROCESSES_MAX];

// TODO: Free acquired resources on failure.
error_t create_app(cid_t kernel_ch, cid_t memmgr_ch, cid_t appmgr_ch,
                   const struct file *file, pid_t *app_pid) {
    error_t err;

    page_base_t page_base = valloc(4096);
    uint8_t *file_header;
    size_t num_pages;
    size_t header_size = 0x1000;
    TRY_OR_PANIC(call_fs_read(file->fs_server, file->fd, 0, header_size,
        page_base, (uintptr_t *) &file_header, &num_pages));

    struct elf_file elf;
    if ((err = elf_parse(&elf, file_header, header_size)) != OK) {
        WARN("Invalid ELF file: %d", err);
        return err;
    }

    pid_t pid;
    cid_t pager_ch;
    if ((err = call_process_create(kernel_ch, "app", &pid, &pager_ch)) != OK) {
        return err;
    }

    if ((err = transfer(pager_ch, appmgr_ch)) != OK) {
        return err;
    }

    cid_t gui_ch;
    TRY_OR_PANIC(call_discovery_connect(memmgr_ch, GUI_INTERFACE, &gui_ch));
    TRY_OR_PANIC(call_process_send_channel(kernel_ch, pid, gui_ch));

    // The executable image.
    if ((err = call_process_add_pager(kernel_ch, pid, 1, APP_IMAGE_START,
                                      APP_IMAGE_SIZE, 0x06)) != OK) {
        return err;
    }

    // The zeroed pages (stack and heap).
    if ((err = call_process_add_pager(kernel_ch, pid, 1, APP_ZEROED_PAGES_START,
                                      APP_ZEROED_PAGES_SIZE, 0x06)) != OK) {
        return err;
    }

    struct process *proc = NULL;
    for (int i = 0; i < NUM_PROCESSES_MAX; i++) {
        if (processes[i].pid == 0) {
            proc = &processes[i];
        }
    }

    if (!proc) {
        WARN("too many processes");
        return ERR_OUT_OF_MEMORY;
    }

    proc->pid = pid;
    proc->file = file;
    proc->pager_ch = pager_ch;
    memcpy(&proc->elf, &elf, sizeof(elf));

    *app_pid = pid;
    return OK;
}

struct process *get_process_by_pid(pid_t pid) {
    for (int i = 0; i < NUM_PROCESSES_MAX; i++) {
        if (processes[i].pid == pid) {
            return &processes[i];
        }
    }

    return NULL;
}

struct process *get_process_by_cid(cid_t cid) {
    for (int i = 0; i < NUM_PROCESSES_MAX; i++) {
        if (processes[i].pager_ch == cid) {
            return &processes[i];
        }
    }

    return NULL;
}

error_t start_app(cid_t kernel_ch, pid_t app_pid) {
    struct process *proc = get_process_by_pid(app_pid);
    assert(proc);

    tid_t tid;
    TRY(call_thread_spawn(kernel_ch, app_pid, proc->elf.entry,
                          APP_INITIAL_STACK_POINTER, 0x1000, 0, &tid));
    return OK;
}

void process_init(void) {
    for (int i = 0; i < NUM_PROCESSES_MAX; i++) {
        processes[i].pid = 0;
    }
}
