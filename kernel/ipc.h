#ifndef __IPC_H__
#define __IPC_H__

#include <types.h>

struct task;
struct message;
MUSTUSE error_t ipc(struct task *dst, task_t src, struct message *m, unsigned flags);
void notify(struct task *dst, notifications_t notifications);

#endif
