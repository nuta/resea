#include <resea/ipc.h>
#include <resea/printf.h>
#include <string.h>
#include <types.h>

static uint32_t seed = 0;

static uint32_t get_random(void) {
    return seed = (seed * 1103515245 + 12345);
}

void main(void) {
    TRACE("starting...");

    ASSERT_OK(ipc_serve("random"));

    INFO("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        ASSERT_OK(ipc_recv(IPC_ANY, &m));

        switch (m.type) {
            case RANDOM_GET_RANDOM_MSG: {
                m.type = RANDOM_GET_RANDOM_REPLY_MSG;
                m.random_get_random_reply.random_number = get_random();
                ipc_reply(m.src, &m);
                break;
            }
            default:
                discard_unknown_message(&m);
        }
    }
}
