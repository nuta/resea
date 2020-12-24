#include "test.h"
#include <resea/ipc.h>
#include <resea/printf.h>
#include <string.h>

void ipc_test(void) {
    struct message m;
    int err;

    // A IPC call.
    for (int i = 0; i < 10; i++) {
        m.type = BENCHMARK_NOP_MSG;
        m.benchmark_nop.value = i;
        err = ipc_call(INIT_TASK, &m);
        TEST_ASSERT(err == OK);
        TEST_ASSERT(m.type == BENCHMARK_NOP_REPLY_MSG);
        TEST_ASSERT(m.benchmark_nop.value == i * 7);
    }

    // A ool IPC call.
    m.type = BENCHMARK_NOP_WITH_OOL_MSG;
    m.benchmark_nop_with_ool.data = "hi!";
    m.benchmark_nop_with_ool.data_len = 3;
    err = ipc_call(INIT_TASK, &m);
    TEST_ASSERT(err == OK);
    TEST_ASSERT(m.type == BENCHMARK_NOP_WITH_OOL_REPLY_MSG);
    TEST_ASSERT(m.benchmark_nop_with_ool_reply.data_len == 7);
    TEST_ASSERT(!memcmp(m.benchmark_nop_with_ool_reply.data, "reply!\0", 7));

    // A ool IPC call.
    static char page[PAGE_SIZE * 2] = {'a', 'b', 'c'};
    m.type = BENCHMARK_NOP_WITH_OOL_MSG;
    m.benchmark_nop_with_ool.data = page;
    m.benchmark_nop_with_ool.data_len = sizeof(page);
    err = ipc_call(INIT_TASK, &m);
    TEST_ASSERT(err == OK);
    TEST_ASSERT(m.type == BENCHMARK_NOP_WITH_OOL_REPLY_MSG);

    // A ool IPC call (with empty payload).
    m.type = BENCHMARK_NOP_WITH_OOL_MSG;
    m.benchmark_nop_with_ool.data = NULL;
    m.benchmark_nop_with_ool.data_len = 0;
    err = ipc_call(INIT_TASK, &m);
    TEST_ASSERT(err == OK);
    TEST_ASSERT(m.type == BENCHMARK_NOP_WITH_OOL_REPLY_MSG);
}
