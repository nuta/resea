#include <print.h>
#include <resea/ipc.h>
#include <string.h>

void main(void) {
    TRACE("ready");
    struct message m;
    for (int i = 0; i < 5; i++) {
        m.type = ADD_MSG;
        m.add.x = i;
        m.add.y = 0xa00000;
        INFO("ping pong %d", i);
        error_t err = ipc_call(2, &m);
        ASSERT_OK(err);
        ASSERT(m.type == ADD_REPLY_MSG);
        ASSERT(m.add_reply.value == i + 0xa00000);
    }

    INFO("Done!");
}
