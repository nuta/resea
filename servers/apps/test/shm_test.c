#include <resea/ipc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <resea/task.h>
#include <string.h>
#include "test.h"

#define MSG "hello task b"

void test() {
    struct message m;
    error_t err;

    // create shared memory
    m.type = SHM_CREATE_MSG;
    m.shm_create.size = 32;
    err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    int shm_id = m.shm_create_reply.shm_id;
    TEST_ASSERT(shm_id > -1);

    // map shm
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = shm_id;
    m.shm_map.flag = 1;
    err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    vaddr_t vaddr = m.shm_map_reply.vaddr;
    TEST_ASSERT(vaddr);

    // err mapping invalid/unused shm
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = 5;  // unused shm_id
    err = ipc_call(INIT_TASK, &m);
    TEST_ASSERT(err == ERR_NOT_FOUND);

    // close shm and test map err
    bzero(&m, sizeof(m));
    m.type = SHM_CLOSE_MSG;
    m.shm_close.shm_id = shm_id;
    ipc_call(INIT_TASK, &m);

    // mapping closed shm
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = shm_id;  // closed shm
    err = ipc_call(INIT_TASK, &m);
    TEST_ASSERT(err == ERR_NOT_FOUND);
}
void readtest(){

    struct message m;
    error_t err;
    task_t shm_test = ipc_lookup("shm_test_server");
    if(shm_test< 0){
        return;
    }

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
    TEST_ASSERT(strcmp(MSG, buf) == 0);

}
void shm_test(void) {
    test();
    readtest();
    }