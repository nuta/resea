#ifndef __IPC_H__
#define __IPC_H__

#include <types.h>

struct task;
struct ipc_msg_t;
error_t ipc(struct task *dst, task_t src, struct ipc_msg_t *m, unsigned flags);
void notify(struct task *dst, notifications_t notifications);

#endif
