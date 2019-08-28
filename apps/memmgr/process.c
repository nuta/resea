#include "process.h"

#include "elf.h"
#include "initfs.h"
#include "memory_map.h"

#include <resea.h>
#include <resea/ipc.h>
#include <resea/string.h>
#include <resea_idl.h>

// TODO: Use hash table.
static struct process processes[NUM_PROCESSES_MAX];

struct process *get_process_by_pid(pid_t pid) {
    for (int i = 0; i < NUM_PROCESSES_MAX; i++) {
        if (processes[i].pid == pid) {
            return &processes[i];
        }
    }

    return NULL;
}

error_t spawn_process(
    cid_t kernel_ch, cid_t server_ch, const struct initfs_file *file) {
    TRACE("spawning '%s'", file->path);
    error_t err;

    struct elf_file elf;
    if ((err = elf_parse(&elf, file->content, file->len)) != OK) {
        return err;
    }

    pid_t pid;
    cid_t pager_ch;
    if ((err = create_process(kernel_ch, &pid, &pager_ch)) != OK) {
        return err;
    }

    if ((err = transfer(pager_ch, server_ch)) != OK) {
        return err;
    }

    // The executable image.
    if ((err = add_pager(
             kernel_ch, pid, 1, APP_IMAGE_START, APP_IMAGE_SIZE, 0x06)) != OK) {
        return err;
    }

    // The zeroed pages (stack and heap).
    if ((err = add_pager(kernel_ch, pid, 1, APP_ZEROED_PAGES_START,
             APP_ZEROED_PAGES_SIZE, 0x06)) != OK) {
        return err;
    }

    tid_t tid;
    if ((err = spawn_thread(kernel_ch, pid, elf.entry,
             APP_INITIAL_STACK_POINTER, 0x1000, 0 /* arg */, &tid)) != OK) {
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
        return ERR_NO_MEMORY;
    }

    proc->pid = pid;
    proc->file = file;
    memcpy(&proc->elf, &elf, sizeof(elf));

    INFO("spawned '%s'", file->path);
    return OK;
}

void process_init(void) {
    for (int i = 0; i < NUM_PROCESSES_MAX; i++) {
        processes[i].pid = 0;
    }
}
