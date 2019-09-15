#include <app.h>
#include <resea.h>
#include <resea_idl.h>
#include <resea/ipc.h>

int main(void) {
    api_console_write("Resea Shell\n");

    cid_t appmgr_ch = 1;
    cid_t gui_ch = 2;
    TRY_OR_PANIC(activate(gui_ch));

    // TODO: Refactor this nasty implementation.
    char cmdline[64];
    int i;
    while (1) {
new_prompt:
        api_console_write("> ");
        i = 0;
        while (1) {
            struct message m;
            TRY_OR_PANIC(ipc_recv(gui_ch, &m));
            switch (MSG_TYPE(m.header)) {
            case KEY_EVENT_MSG: {
                struct key_event_msg *m2 = (struct key_event_msg *) &m;
                if (m2->ch == 0 /* KEY_ENTER */) {
                    console_write(gui_ch, '\n');
                    cmdline[i] = '\0';
                    pid_t child;
                    if (api_create_app(appmgr_ch, cmdline, &child) != OK) {
                        api_console_write("Error: failed to launch the app\n");
                        goto new_prompt;
                    }
                    TRY_OR_PANIC(api_start_app(appmgr_ch, child));
                    int8_t exit_code;
                    TRY_OR_PANIC(api_join_app(appmgr_ch, child, &exit_code));
                    goto new_prompt;
                } else if (0x20 <= m2->ch && m2->ch < 0x80) {
                    console_write(gui_ch, m2->ch);
                    cmdline[i++] = m2->ch;
                    if (i == sizeof(cmdline) - 1) {
                        api_console_write("\nError: Too long input.\n");
                        goto new_prompt;
                    }
                }
                break;
            }
            default:
                WARN("invalid message type %x", MSG_TYPE(m.header));
            }
        }
    }

    return 0;
}
