#pragma once
#include <message.h>
#include <types.h>

error_t ipc_send(task_t dst, struct message *m);
error_t ipc_send_noblock(task_t dst, struct message *m);
void ipc_reply(task_t dst, struct message *m);
void ipc_reply_err(task_t dst, error_t error);
error_t ipc_recv(task_t src, struct message *m);
error_t ipc_call(task_t dst, struct message *m);
error_t ipc_notify(task_t dst, notifications_t notifications);
