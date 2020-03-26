#ifndef __SESSION_H__
#define __SESSION_H__

#include <list.h>
#include <types.h>
#include <message.h>

#define SESSIONS_MAX 256

void session_init(void);
handle_t session_new(void);
error_t session_alloc(handle_t handle);
void session_delete(handle_t handle);
void *session_get(handle_t handle);
void session_set(handle_t handle, void *data);

#endif
