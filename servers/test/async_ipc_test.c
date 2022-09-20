#include <print.h>
#include <resea/ipc.h>
#include <resea/test.h>
#include <string.h>

#define TEST_SERVER 1

UNIT_TEST("async IPC") {
    struct message m;
    m.type = ASYNC_PING_MSG;
    ipc_send(TEST_SERVER, &m);

    memset(&m, 0, sizeof(m));
    ASSERT_OK(ipc_recv(IPC_ANY, &m));

    EXPECT_EQ(m.type, ASYNC_PONG_MSG);
    EXPECT_EQ(m.async_pong.a, 42);
}
