#ifndef __RESEA_TASK_H__
#define __RESEA_TASK_H__

#include <types.h>

error_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                    unsigned flags);
error_t task_destroy(task_t task);
__noreturn void task_exit(void);
task_t task_self(void);
error_t vm_map(task_t task, vaddr_t vaddr, vaddr_t src, vaddr_t kpage,
               unsigned flags);
error_t vm_unmap(task_t task, vaddr_t vaddr);
error_t task_schedule(task_t task, int priority);

#endif
