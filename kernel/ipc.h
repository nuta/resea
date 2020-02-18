#ifndef __IPC_H__
#define __IPC_H__

#include <types.h>

struct task;
error_t ipc(struct task *dst, tid_t src, tid_t send_as, uint32_t flags);
struct message;
error_t kernel_ipc(struct task *dst, tid_t src, struct message *m,
                   uint32_t ops);
void notify(struct task *dst, notifications_t notifications);

#endif
