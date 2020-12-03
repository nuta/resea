
#include <resea/ipc.h>
#include "test.h"
void shm_test(void) {
    struct message m;
    int err;
    m.type = SHM_CREATE_MSG;
    m.shm_create.size = 2;

    err = ipc_call(INIT_TASK, &m);
    int shm_id = m.shm_create_reply.shm_id;
    TEST_ASSERT(shm_id > -1);

    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = shm_id;
    err = ipc_call(INIT_TASK, &m);
    //INFO("vaddr = %d", m.shm_map_reply.vaddr);
    TEST_ASSERT(m.shm_map_reply.vaddr);
}