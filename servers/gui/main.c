#include <resea/ipc.h>
#include <resea/printf.h>
#include <resea/timer.h>

void main(void) {
    TRACE("ready");

    ASSERT_OK(ipc_serve("gui"));

    // The mainloop: receive and handle messages.
    INFO("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case NOTIFICATIONS_MSG:
                break;
        }
    }
}
