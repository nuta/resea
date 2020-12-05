#include <resea/ipc.h>
#include <resea/printf.h>
#include <string.h>

#define TEST_DATA "hello task"

void main(void) {
    ASSERT_OK(ipc_serve("shm_test_server"));
    struct message m;
    error_t err;
    // create shared memory
    m.type = SHM_CREATE_MSG;
    m.shm_create.size = 100;
    err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    int shm_id = m.shm_create_reply.shm_id;
    ASSERT_OK(shm_id);
    INFO("shm_id: %d", shm_id);
    // map shm
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = shm_id;
    m.shm_map.writable = true;
    err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    vaddr_t vaddr = m.shm_map_reply.vaddr;
    INFO("vaddr : %ld", vaddr);
    // write to vaddr
    char buf[32] = TEST_DATA;
    memcpy((void*) vaddr, buf, sizeof(buf));
    while (true) {
        bzero(&m, sizeof(m));
        err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case SHM_TEST_READ_MSG:
                m.type = SHM_TEST_READ_REPLY_MSG;
                m.shm_test_read_reply.shm_id = shm_id;
                ipc_reply(m.src, &m);
                break;
            default:
                discard_unknown_message(&m);
                break;
        }
    }
}
