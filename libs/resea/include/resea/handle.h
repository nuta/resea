#ifndef __RESEA_HANDLE_H__
#define __RESEA_HANDLE_H__

#include <types.h>

#define HANDLES_MAX 128

void handle_init(void);
handle_t handle_alloc(void);
void *handle_get(handle_t handle);
void handle_set(handle_t handle, void *data);
void handle_free(handle_t handle);

#endif
