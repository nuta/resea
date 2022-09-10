#pragma once
#include <types.h>

struct message;

error_t sys_ipc(task_t dst, task_t src, struct message *m, unsigned flags);
task_t sys_task_create(const char *name, vaddr_t ip, task_t pager,
                       unsigned flags);
error_t sys_task_destroy(task_t task);
error_t sys_task_exit(void);
task_t sys_task_self(void);
paddr_t sys_pm_alloc(size_t size, unsigned flags);
error_t sys_vm_map(task_t task, uaddr_t uaddr, paddr_t paddr, size_t size,
                   unsigned attrs);
error_t sys_vm_unmap(task_t task, uaddr_t uaddr, size_t size);
error_t sys_console_write(const char *buf, size_t len);
int sys_console_read(const char *buf, int max_len);
