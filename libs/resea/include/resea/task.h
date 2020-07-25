#ifndef __IPC_TASK_H__
#define __IPC_TASK_H__

#include <types.h>

error_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                   unsigned flags);
error_t task_destroy(task_t task);
void task_exit(void);
task_t task_self(void);
error_t task_map(task_t task, vaddr_t vaddr, vaddr_t src, vaddr_t kpage,
                 unsigned flags);

#endif
