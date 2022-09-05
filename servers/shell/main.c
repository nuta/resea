#include <print_macros.h>
#include <resea/syscall.h>
#include <strlen.h>

// FIXME:
void printf_flush(void);

void main(void) {
    INFO("Hello World from shell!");
    while (true) {
        char cmdline[256];
        printf("shell> ");

        while (true) {
            char buf[8];
            int read_len = sys_console_read(buf, sizeof(buf));
            if (read_len < 0) {
                WARN("console_read failed: %d", read_len);
                continue;
            }

            for (int i = 0; i < read_len; i++) {
                if (buf[i] == '\r') {
                    continue;
                }

                printf("%c", buf[i]);
                printf_flush();

                if (buf[i] == '\n') {
                    cmdline[0] = '\0';
                    break;
                }

                cmdline[strlen(cmdline)] = buf[i];
                cmdline[strlen(cmdline) + 1] = '\0';
            }
        }

        if (strlen(cmdline) == 0) {
            continue;
        }

        DBG("cmdline: %s", cmdline);
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
