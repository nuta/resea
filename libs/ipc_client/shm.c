#include "helper.h"
#include <ipc_client/shm.h>
#include <resea/ipc.h>

error_t shm_create(size_t size, handle_t *handle, void **ptr) {
    struct message m;
    m.type = SHM_CREATE_MSG;
    m.shm_create.size = size;
    CHECKED_IPC_CALL(lookup_vm_task(), &m, SHM_CREATE_REPLY_MSG);
    *handle = m.shm_create_reply.handle;
    *ptr = (void *) m.shm_create_reply.vaddr;
    return OK;
}

error_t shm_map(handle_t handle, bool writable, void **ptr) {
    struct message m;
    m.type = SHM_MAP_MSG;
    m.shm_map.handle = handle;
    m.shm_map.writable = writable;
    CHECKED_IPC_CALL(lookup_vm_task(), &m, SHM_MAP_REPLY_MSG);
    *ptr = (void *) m.shm_map_reply.vaddr;
    return OK;
}
