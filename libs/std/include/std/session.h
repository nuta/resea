#ifndef __SESSION_H__
#define __SESSION_H__

#include <list.h>
#include <types.h>
#include <message.h>

#define HANDLES_MAX 256

struct session {
    list_elem_t next;
    task_t owner;
    handle_t handle;
    void *data;
};

void session_init(void);
struct session *session_alloc(task_t task);
struct session *session_alloc_at(task_t task, handle_t handle);
struct session *session_get(task_t task, handle_t handle);
struct session *session_delete(task_t task, handle_t handle);

#endif
