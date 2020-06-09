#include <resea/printf.h>
#include <resea/ipc.h>
#include <cstring.h>
#include "test.h"

void ipc_test(void) {
    struct message m;
    int err;

    // A IPC call.
    for (int i = 0; i < 10; i++) {
        m.type = NOP_MSG;
        m.nop.value = i;
        err = ipc_call(INIT_TASK_TID, &m);
        TEST_ASSERT(err == OK);
        TEST_ASSERT(m.type == NOP_REPLY_MSG);
        TEST_ASSERT(m.nop.value == i * 7);
    }

    // A bulk IPC call.
    m.type = NOP_WITH_BULK_MSG;
    m.nop_with_bulk.data = "hi!";
    m.nop_with_bulk.data_len = 3;
    err = ipc_call(INIT_TASK_TID, &m);
    TEST_ASSERT(err == OK);
    TEST_ASSERT(m.type == NOP_WITH_BULK_REPLY_MSG);
    TEST_ASSERT(m.nop_with_bulk_reply.data_len == 7);
    TEST_ASSERT(!memcmp(m.nop_with_bulk_reply.data, "reply!\0", 7));

    // A bulk IPC call.
    static char page[PAGE_SIZE * 4] = {'a', 'b', 'c'};
    m.type = NOP_WITH_BULK_MSG;
    m.nop_with_bulk.data = page;
    m.nop_with_bulk.data_len = sizeof(page);
    err = ipc_call(INIT_TASK_TID, &m);
    TEST_ASSERT(err == OK);
    TEST_ASSERT(m.type == NOP_WITH_BULK_REPLY_MSG);

     // A bulk IPC call (with empty payload).
    m.type = NOP_WITH_BULK_MSG;
    m.nop_with_bulk.data = NULL;
    m.nop_with_bulk.data_len = 0;
    err = ipc_call(INIT_TASK_TID, &m);
    TEST_ASSERT(err == OK);
    TEST_ASSERT(m.type == NOP_WITH_BULK_REPLY_MSG);
}
