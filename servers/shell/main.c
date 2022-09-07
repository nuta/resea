#include <print_macros.h>
#include <resea/syscall.h>
#include <string.h>

// FIXME:
void printf_flush(void);

void main(void) {
    while (true) {
        char cmdline[256];
        printf("\x1b[1mshell> \x1b[0m");
        printf_flush();

        while (true) {
            char buf[8];
            int read_len = sys_console_read(buf, sizeof(buf));
            if (read_len < 0) {
                WARN("console_read failed: %d", read_len);
                continue;
            }

            for (int i = 0; i < read_len; i++) {
                if (buf[i] == '\r') {
                    buf[i] = '\n';  // TODO:
                }
                printf("%c", buf[i]);
                printf_flush();

                if (buf[i] == '\n') {
                    goto done;
                }

                cmdline[strlen(cmdline)] = buf[i];
                cmdline[strlen(cmdline) + 1] = '\0';
            }
        }

done:
        if (strlen(cmdline) == 0) {
            continue;
        }

        // parse cmdline
        char *argv[16];
        int argc = 0;
        char *p = cmdline;
        while (*p != '\0') {
            while (*p == ' ') {
                p++;
            }

            if (*p == '\0') {
                break;
            }

            argv[argc++] = p;
            while (*p != ' ' && *p != '\0') {
                p++;
            }

            if (*p == '\0') {
                break;
            }

            *p = '\0';
            p++;

            if (argc >= 16) {
                break;
            }
        }

        if (argc == 0) {
            continue;
        }

        // execute the command
        if (!strncmp(argv[0], "echo", 4)) {
            for (int i = 1; i < argc; i++) {
                printf("%s ", argv[i]);
            }
            printf("\n");
        }
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
