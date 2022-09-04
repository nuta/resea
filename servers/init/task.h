#pragma once
#include <types.h>

struct task {
    task_t tid;
    task_t pager;
    void *file_header;
};

task_t task_spawn(struct bootfs_file *file, const char *cmdline);
