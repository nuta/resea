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

// Extracts the file name without the extension. E.g., "startups/gui.elf"
// -> "elf".
static void copy_process_name_from_path(char *buf, size_t buf_len,
                                        const char *path) {
    if (strlen(path) == 0) {
        goto invalid_path;
    }

    int pos = strlen(path) - 1;
    // Find the last '.'.
    int dot_pos = 0;
    while (pos > 0) {
        if (path[pos] == '.') {
            dot_pos = pos;
            break;
        }

        pos--;
    }

    if (pos < 0) {
        goto invalid_path;
    }

    // Find the last '/'.
    int slash_pos = dot_pos;
    while (pos > 0) {
        if (path[pos] == '/') {
            slash_pos = pos;
            break;
        }

        pos--;
    }

    if (pos < 0) {
        goto invalid_path;
    }

    // Copy the name.
    strncpy_s(buf, buf_len, &path[slash_pos + 1], dot_pos - slash_pos - 1);
    return;

invalid_path:
    strcpy_s(buf, buf_len, "(invalid path)");
}

error_t spawn_process(cid_t kernel_ch, cid_t server_ch,
                      const struct initfs_file *file) {
    TRACE("spawning '%s'", file->path);
    error_t err;

    struct elf_file elf;
    if ((err = elf_parse(&elf, file->content, file->len)) != OK) {
        return err;
    }

    char name[32];
    copy_process_name_from_path((char *) &name, 32, file->path);

    pid_t pid;
    cid_t pager_ch;
    if ((err = create_process(kernel_ch, (const char *) &name, &pid,
                              &pager_ch)) != OK) {
        return err;
    }

    cid_t user_kernel_ch;
    if ((err = add_kernel_channel(kernel_ch, pid, &user_kernel_ch))
        != OK) {
        return err;
    }
    assert(user_kernel_ch == 2);

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
