#ifndef __RESEA_HANDLE_H__
#define __RESEA_HANDLE_H__

#include <types.h>

handle_t handle_alloc(task_t owner);
void *handle_get(task_t owner, handle_t handle);
void handle_set(task_t owner, handle_t handle, void *data);
void handle_free(task_t owner, handle_t handle);
void handle_free_all(task_t owner, void (*before_free)(void *data));

#endif
