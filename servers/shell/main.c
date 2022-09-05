#include <print_macros.h>
#include <resea/syscall.h>

void main(void) {
    INFO("Hello World from shell!");
    while (true) {
        char buf[256];
        sys_console_read(buf, sizeof(buf));
        printf("read: %s\n");
    }

    // while (true) {
    //     struct message m;
    //     error_t err = ipc_recv_any(&m);
    //     ASSERT_OK(err);

    //     switch (m.type) {
    //         case PING_MSG:
    //             INFO("Got ping from %d", m.src);
    //             m.type = PONG_MSG;
    //             err = ipc_reply(m.src, &m);
    //             break;
    //         case EXIT_MSG:
    //             INFO("Got exit from %d", m.src);
    //             kill(m.src);
    //             break;
    //     }
    // }
}
