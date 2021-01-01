#ifndef __IPC_H__
#define __IPC_H__

#include "syscall.h"
#include <types.h>

struct task;
struct message;
__mustuse error_t ipc(struct task *dst, task_t src, __user struct message *m,
                      unsigned flags);
void notify(struct task *dst, notifications_t notifications);

#endif
