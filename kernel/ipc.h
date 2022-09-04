#pragma once
#include <message.h>

struct task;

error_t ipc(struct task *dst, task_t src, __user struct message *m,
            unsigned flags);
