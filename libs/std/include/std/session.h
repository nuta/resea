#ifndef __SESSION_H__
#define __SESSION_H__

#include <list.h>
#include <types.h>
#include <message.h>

#define HANDLES_MAX 256

struct session {
    list_elem_t next;
    tid_t owner;
    handle_t handle;
    void *data;
};

void session_init(void);
struct session *session_alloc(tid_t task);
struct session *session_alloc_at(tid_t task, handle_t handle);
struct session *session_get(tid_t task, handle_t handle);
struct session *session_delete(tid_t task, handle_t handle);

#endif
