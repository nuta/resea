#include <resea/ipc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <resea/task.h>
#include <string.h>
#include "test.h"

void shm_test(void) {
    struct message m;
    error_t err;
    task_t shm_test = ipc_lookup("shm_test_server");
    INFO("shm_test_server : %d", shm_test);

    // request read
    m.type = SHM_TEST_READ_MSG;
    err = ipc_call(shm_test, &m);
    ASSERT_OK(err);
    int shm_id = m.shm_test_read_reply.shm_id;

    // map shm
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = shm_id;
    err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    vaddr_t vaddr = m.shm_map_reply.vaddr;

    // read from vaddr
    char buf[32];
    memcpy(buf, (void *) vaddr, sizeof(buf));
    INFO("read :%s", buf);
}