#include <print.h>
#include <resea/ipc.h>
#include <resea/test.h>
#include <string.h>

#define TEST_SERVER 1

// 53-bytes long string (including the null terminator).
#define TEST_TEXT "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

UNIT_TEST("IPC ping pong") {
    struct message m;
    m.type = PING_MSG;
    strncpy2((char *) m.ping.c, TEST_TEXT, sizeof(m.ping.c));
    m.ping.c_len = 53;
    m.ping.a = 42;
    m.ping.b = -42;
    m.ping.d = 0xa5a5a5a5;

    ipc_call(TEST_SERVER, &m);

    EXPECT_EQ(m.type, PING_REPLY_MSG);
    EXPECT_EQ(m.ping_reply.a, 42);
    EXPECT_EQ(m.ping_reply.b, -42);
    EXPECT_EQ(m.ping_reply.d, 0xa5a5a5a5);
    EXPECT_EQ(m.ping_reply.c_len, 53);
    EXPECT_EQ(strncmp((char *) m.ping_reply.c, TEST_TEXT, 53), 0);
}
