#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <arch/syscall.h>
#include <types.h>

// System calls.
struct ipc_msg_t;
error_t ipc(task_t dst, task_t src, struct ipc_msg_t *m, unsigned flags);
error_t ipcctl(const void *bulk_ptr, size_t bulk_len, msec_t timeout);
task_t taskctl(task_t tid, const char *name, vaddr_t ip, task_t page, caps_t caps);
error_t irqctl(unsigned irq, bool enable);
int klogctl(int op, char *buf, size_t buf_len);

// Wrapper functions.
task_t task_create(task_t tid, const char *name, vaddr_t ip, task_t pager,
                  caps_t caps);
error_t task_destroy(task_t tid);
void task_exit(void);
task_t task_self(void);
void caps_drop(caps_t caps);
error_t ipc_send(task_t dst, struct ipc_msg_t *m);
error_t ipc_send_noblock(task_t dst, struct ipc_msg_t *m);
void ipc_reply(task_t dst, struct ipc_msg_t *m);
void ipc_reply_err(task_t dst, error_t error);
error_t ipc_notify(task_t dst, notifications_t notifications);
error_t ipc_recv(task_t src, struct ipc_msg_t *m);
error_t ipc_call(task_t dst, struct ipc_msg_t *m);
error_t ipc_send_err(task_t dst, error_t error);
error_t timer_set(msec_t timeout);
error_t irq_acquire(unsigned irq);
error_t irq_release(unsigned irq);
void klog_write(const char *str, int len);
int klog_read(char *buf, int len);
error_t klog_listen(void);
error_t klog_unlisten(void);

#endif
