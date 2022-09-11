#include "task.h"
#include "bootfs.h"
#include <elf.h>
#include <print.h>
#include <resea/malloc.h>
#include <resea/syscall.h>
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

    void *file_header = malloc(PAGE_SIZE);

    bootfs_read(file, 0, file_header, PAGE_SIZE);

    // Ensure that it's an ELF file.
    elf_ehdr_t *ehdr = (elf_ehdr_t *) file_header;
    if (memcmp(ehdr->e_ident, ELF_MAGIC, 4) != 0) {
        WARN("%s: invalid ELF magic", file->name);
        return ERR_INVALID_ARG;
    }

    // Ensure that it's an executable file.
    if (ehdr->e_type != ET_EXEC) {
        WARN("%s: not an executable file", file->name);
        return ERR_INVALID_ARG;
    }

    // Create a new task for the server.
    task_t tid_or_err = task_create(file->name, ehdr->e_entry, task_self(), 0);
    if (IS_ERROR(tid_or_err)) {
        return tid_or_err;
    }

    task->file = file;
    task->file_header = file_header;
    task->tid = tid_or_err;
    task->pager = task_self();
    task->ehdr = ehdr;
    task->phdrs = (elf_phdr_t *) ((uintptr_t) file_header + ehdr->e_phoff);

    // Copy the task name.
    size_t i = 0;
    for (; i < sizeof(task->name) - 1 && file->name[i] != '\0'; i++) {
        task->name[i] = file->name[i];
    }
    task->name[i] = '\0';

    // Register the created task in the TID table.
    tasks[task->tid - 1] = task;
    return task->tid;
}

void task_destroy2(struct task *task) {
    ASSERT_OK(task_destroy(task->tid));
    free(task->file_header);
    free(task);
    tasks[task_self() - 1] = NULL;
}
