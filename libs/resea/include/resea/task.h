#pragma once
#include <types.h>

error_t task_create(const char *name, vaddr_t ip, task_t pager, unsigned flags);
error_t task_destroy(task_t task);
__noreturn void task_exit(void);
task_t task_self(void);
