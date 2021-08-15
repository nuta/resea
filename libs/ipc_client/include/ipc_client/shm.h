#ifndef __IPC_CLIENT_SHM_H__
#define __IPC_CLIENT_SHM_H__

#include <types.h>

error_t shm_create(size_t size, handle_t *handle, void **ptr);
error_t shm_map(handle_t handle, bool writable, void **ptr);

#endif
