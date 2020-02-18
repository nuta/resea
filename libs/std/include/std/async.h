#ifndef __STD_ASYNC_H__
#define __STD_ASYNC_H__

#include <list.h>
#include <types.h>

struct async_message {
    tid_t dst;
    struct message *m;
    size_t len;
    list_elem_t next;
};

void async_send(tid_t dst, struct message *m, size_t len);
void async_flush(void);
void async_init(void);

#endif
