#ifndef __RESEA_SYSCALL_H__
#define __RESEA_SYSCALL_H__

#include <arch/syscall.h>
#include <types.h>

// System calls.
struct message;

// Wrapper functions.
error_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                   caps_t caps);
error_t task_destroy(task_t task);
void task_exit(void);
task_t task_self(void);
error_t ipc_send(task_t dst, struct message *m);
error_t ipc_send_noblock(task_t dst, struct message *m);
void ipc_reply(task_t dst, struct message *m);
void ipc_reply_err(task_t dst, error_t error);
error_t ipc_notify(task_t dst, notifications_t notifications);
error_t ipc_recv(task_t src, struct message *m);
error_t ipc_call(task_t dst, struct message *m);
error_t ipc_send_err(task_t dst, error_t error);
error_t timer_set(msec_t timeout);
error_t irq_acquire(unsigned irq);
error_t irq_release(unsigned irq);
error_t klog_write(const char *buf, size_t len);
int klog_read(char *buf, size_t len, bool listen);
void nop_syscall(void);

#endif
