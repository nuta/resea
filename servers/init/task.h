#pragma once
#include <elf.h>
#include <types.h>

struct task {
    task_t tid;
    task_t pager;
    char name[TASK_NAME_LEN];
    void *file_header;
    elf_ehdr_t *ehdr;
    elf_phdr_t *phdrs;
};

struct bootfs_file;

struct task *task_lookup(task_t tid);
task_t task_spawn(struct bootfs_file *file, const char *cmdline);
