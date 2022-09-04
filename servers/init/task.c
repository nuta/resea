#include "task.h"
#include "bootfs.h"
#include <elf.h>
#include <print_macros.h>
#include <resea/malloc.h>
#include <resea/syscall.h>
#include <resea/task.h>
#include <string.h>

static struct task *tasks[NUM_TASKS_MAX];

// FIXME:
__aligned(PAGE_SIZE) uint8_t tmp_page[PAGE_SIZE];

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

    bootfs_read(file, 0, file_header, PAGE_SIZE);

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

    task->file = file;
    task->file_header = file_header;
    task->tid = tid_or_err;
    task->pager = task_self();
    task->ehdr = ehdr;
    task->phdrs = (elf_phdr_t *) ((uintptr_t) file_header + ehdr->e_phoff);
    task->name[0] = '\0';  // FIXME: copy
    tasks[task->tid - 1] = task;

    for (uint16_t i = 0; i < ehdr->e_phnum; i++) {
        elf_phdr_t *phdr = &task->phdrs[i];
        // Ignore GNU_STACK
        if (!phdr->p_vaddr) {
            continue;
        }

        ASSERT(phdr->p_memsz >= phdr->p_filesz);

        TRACE("bootelf: %p - %p (%d KiB)", phdr->p_vaddr,
              phdr->p_vaddr + ALIGN_UP(phdr->p_memsz, PAGE_SIZE),
              phdr->p_memsz / 1024);

        for (size_t off = 0; off < phdr->p_memsz; off += PAGE_SIZE) {
            void *tmp_page = (void *) 0xa000;
            paddr_t paddr = sys_pm_alloc(PAGE_SIZE, 0);
            ASSERT_OK(sys_vm_map(sys_task_self(), (uaddr_t) tmp_page, paddr,
                                 PAGE_SIZE,
                                 /*FIXME: */ 0));

            if (off < phdr->p_filesz) {
                bootfs_read(task->file, phdr->p_offset + off, (void *) tmp_page,
                            MIN(PAGE_SIZE, phdr->p_filesz - off));
            }

            sys_vm_map(task->tid, phdr->p_vaddr + off, paddr, PAGE_SIZE,
                       /*FIXME: */ 0);
        }
    }

    return task->tid;
}
