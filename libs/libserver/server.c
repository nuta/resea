#include <resea.h>
#include <resea_idl.h>
#include <server.h>

error_t server_mainloop_with_deferred(cid_t ch,
                                      error_t (*process)(struct message *m),
                                      void (*deferred_work)(void)) {
    struct message m;
    while (1) {
        // Receive a message from a client.
        TRY(ipc_recv(ch, &m));
        // Do work and fill a reply message into `m`.
        error_t err = process(&m);
        // Warn if the handler returned ERR_UNEXPECTED_MESSAGE since it is
        // likely that the handler simply does not yet implement it.
        if (err == ERR_UNEXPECTED_MESSAGE) {
            WARN("unexpected message type: %x", MSG_TYPE(m.header));
        }
        // If the handler returned ERR_DONT_REPLY, we don't reply.
        if (err != ERR_DONT_REPLY) {
            // If the handler returned an error, reply an error message.
            if (err != OK) {
                m.header = ERROR_TO_HEADER(err);
            }
            // Send the reply message.
            TRY(ipc_send(m.from, &m));
        }
        // Do the deferred work if specified.
        if (deferred_work) {
            deferred_work();
        }
    }
}

error_t server_mainloop(cid_t ch, error_t (*process)(struct message *m)) {
    return server_mainloop_with_deferred(ch, process, NULL);
}

error_t server_register(cid_t discovery_server, cid_t receive_at,
                        uint16_t interface, cid_t *new_ch) {
    cid_t ch;
    TRY(open(&ch));
    TRY(transfer(ch, receive_at));
    TRY(call_discovery_publicize(discovery_server, interface, ch));
    *new_ch = ch;
    return OK;
}
