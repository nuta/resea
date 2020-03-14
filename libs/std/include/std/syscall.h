#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <arch/syscall.h>
#include <types.h>

// System calls.
struct message;
error_t ipc(tid_t dst, tid_t src, struct message *m, unsigned flags);
error_t ipcctl(const void *bulk_ptr, size_t bulk_len, msec_t timeout);
tid_t taskctl(tid_t tid, const char *name, vaddr_t ip, tid_t page, caps_t caps);
error_t irqctl(unsigned irq, bool enable);
int klogctl(int op, char *buf, size_t buf_len);

// Wrapper functions.
tid_t task_create(tid_t tid, const char *name, vaddr_t ip, tid_t pager,
                  caps_t caps);
error_t task_destroy(tid_t tid);
void task_exit(void);
tid_t task_self(void);
void caps_drop(caps_t caps);
error_t ipc_send(tid_t dst, struct message *m);
error_t ipc_send_noblock(tid_t dst, struct message *m);
void ipc_reply(tid_t dst, struct message *m);
void ipc_reply_err(tid_t dst, error_t error);
error_t ipc_notify(tid_t dst, notifications_t notifications);
error_t ipc_recv(tid_t src, struct message *m);
error_t ipc_call(tid_t dst, struct message *m);
error_t ipc_send_err(tid_t dst, error_t error);
error_t timer_set(msec_t timeout);
error_t irq_acquire(unsigned irq);
error_t irq_release(unsigned irq);
void klog_write(const char *str, int len);
int klog_read(char *buf, int len);
error_t klog_listen(void);
error_t klog_unlisten(void);

#endif
