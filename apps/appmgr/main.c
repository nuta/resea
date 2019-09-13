#include <resea.h>
#include <resea_idl.h>

static void mainloop(cid_t server_ch) {
    struct message m;
    TRY_OR_PANIC(ipc_recv(server_ch, &m));
    while (1) {
        struct message r;
        error_t err;
        cid_t from = m.from;
        switch (MSG_TYPE(m.header)) {
        default:
            WARN("invalid message type %x", MSG_TYPE(m.header));
            err = ERR_INVALID_MESSAGE;
            r.header = ERROR_TO_HEADER(err);
        }

        if (err == ERR_DONT_REPLY) {
            TRY_OR_PANIC(ipc_recv(server_ch, &m));
        } else {
            TRY_OR_PANIC(ipc_replyrecv(from, &r, &m));
        }
    }
}

int main(void) {
    INFO("starting...");
    cid_t server_ch;
    TRY_OR_PANIC(open(&server_ch));

    cid_t memmgr_ch = 1;
    cid_t gui_ch;
    TRY_OR_PANIC(connect_server(memmgr_ch, GUI_INTERFACE, &gui_ch));
    INFO("connected!");
    for (char *s = "Hello from appmgr!"; *s; s++)
        TRY_OR_PANIC(console_write(gui_ch, *s));

    INFO("entering the mainloop...");
    mainloop(server_ch);
    return 0;
}
