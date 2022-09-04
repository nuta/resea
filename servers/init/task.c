#include "task.h"
#include "bootfs.h"
#include <elf.h>
#include <print_macros.h>
#include <resea/malloc.h>
#include <resea/task.h>
#include <string.h>

static struct task *tasks[NUM_TASKS_MAX];

/// Look for the task in the our task table.
struct task *task_lookup(task_t tid) {
    if (tid <= 0 || tid > NUM_TASKS_MAX) {
        PANIC("invalid tid %d", tid);
    }

    return tasks[tid - 1];
}

/// Execute a ELF file. Returns an task ID on success or an error on failure.
task_t task_spawn(struct bootfs_file *file, const char *cmdline) {
    TRACE("launching %s...", file->name);
    struct task *task = malloc(sizeof(*task));
    if (!task) {
        PANIC("too many tasks");
    }

    // FIXME: void *file_header = malloc(PAGE_SIZE);
    void *file_header = malloc(PAGE_SIZE);

    read_file(file, 0, file_header, PAGE_SIZE);

    // Ensure that it's an ELF file.
    elf_ehdr_t *ehdr = (elf_ehdr_t *) file_header;
    if (memcmp(ehdr->e_ident,
               "\x7f"
               "ELF",
               4)
        != 0) {
        WARN("%s: invalid ELF magic, ignoring...", file->name);
        return ERR_INVALID_ARG;
    }

    // Create a new task for the server.
    task_t tid_or_err = task_create(file->name, ehdr->e_entry, task_self(), 0);
    if (IS_ERROR(tid_or_err)) {
        return tid_or_err;
    }

    task->file_header = file_header;
    task->tid = tid_or_err;
    task->pager = task_self();
    tasks[task->tid - 1] = task;

    return task->tid;
}
