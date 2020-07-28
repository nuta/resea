#ifndef __RESEA_ASYNC_H__
#define __RESEA_ASYNC_H__

#include <types.h>
#include <list.h>
#include <message.h>

struct async_message {
    list_elem_t next;
    task_t dst;
    struct message m;
};

error_t async_send(task_t dst, struct message *m);
error_t async_recv(task_t src, struct message *m);
error_t async_reply(task_t dst);

#endif
