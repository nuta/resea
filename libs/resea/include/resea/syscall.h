#ifndef __RESEA_SYSCALL_H__
#define __RESEA_SYSCALL_H__

#include <types.h>

struct message;
error_t sys_ipc(task_t dst, task_t src, struct message *m, unsigned flags);
error_t sys_notify(task_t dst, notifications_t notifications);
error_t sys_timer_set(msec_t timeout);
task_t sys_task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                       unsigned flags);
error_t sys_task_destroy(task_t task);
error_t sys_task_exit(void);
task_t sys_task_self(void);
error_t sys_task_schedule(task_t task, int priority);
error_t sys_vm_map(task_t task, vaddr_t vaddr, vaddr_t src, vaddr_t kpage,
                   unsigned flags);
error_t sys_vm_unmap(task_t task, vaddr_t vaddr);
error_t sys_irq_acquire(unsigned irq);
error_t sys_irq_release(unsigned irq);
error_t sys_console_write(const char *buf, size_t len);
int sys_console_read(char *buf, size_t len);
error_t sys_kdebug(const char *cmd, size_t cmd_len, char *buf, size_t buf_len);

#endif
