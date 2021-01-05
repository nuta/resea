#include <resea/ipc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <resea/task.h>
#include <string.h>
#include "test.h"

#define TEST_DATA "hello task"

void shm_util_test(void) {
    struct message m;
    error_t err;
    // Create a shared memory
    m.type = SHM_CREATE_MSG;
    m.shm_create.size = 32;
    err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    int shm_id = m.shm_create_reply.shm_id;
    TEST_ASSERT(shm_id > -1);
    // Map Shared Memory to task
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = shm_id;
    m.shm_map.writable = true;
    err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    vaddr_t vaddr = m.shm_map_reply.vaddr;
    TEST_ASSERT(vaddr);
    // Mapping Invalid/Unused Shared Memory will lead to Error
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = 5;  // unused shm_id
    err = ipc_call(INIT_TASK, &m);
    TEST_ASSERT(err == ERR_NOT_FOUND);
    // Trying to map closed Shared Memory will lead to Error
    bzero(&m, sizeof(m));
    m.type = SHM_CLOSE_MSG;
    m.shm_close.shm_id = shm_id;
    ipc_call(INIT_TASK, &m);
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = shm_id;  // closed shm
    err = ipc_call(INIT_TASK, &m);
    TEST_ASSERT(err == ERR_NOT_FOUND);
}

void shm_access_test(void) {
    struct message m;
    error_t err;
    task_t shm_test = ipc_lookup("shm_test_server");
    if (shm_test < 0) {
        return;
    }
    // Send read request to shm_test_server
    m.type = SHM_TEST_READ_MSG;
    err = ipc_call(shm_test, &m);
    ASSERT_OK(err);
    int shm_id = m.shm_test_read_reply.shm_id;
    // Map Shared Memory to task
    bzero(&m, sizeof(m));
    m.type = SHM_MAP_MSG;
    m.shm_map.shm_id = shm_id;
    err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    vaddr_t vaddr = m.shm_map_reply.vaddr;
    // Read from shared Memory
    char buf[32];
    memcpy(buf, (void *) vaddr, sizeof(buf));
    TEST_ASSERT(strcmp(TEST_DATA, buf) == 0);
}

void shm_test(void) {
    shm_util_test();
    shm_access_test();
}
