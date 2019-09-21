#include <resea.h>
#include <server.h>

error_t server_mainloop(cid_t ch, error_t (*process)(struct message *m)) {
    struct message m;
    while (1) {
        // Receive a message from a client.
        TRY(ipc_recv(ch, &m));
        // Do work and fill a reply message into `m`.
        error_t err = process(&m);
        // Warn if the handler returned ERR_INVALID_MESSAGE since it is likely
        // that the handler simply does not yet implement it.
        if (err == ERR_INVALID_MESSAGE) {
            OOPS("invalid message type: %x", MSG_TYPE(m.header));
        }
        // If the handler returned ERR_DONT_REPLY, we don't reply.
        if (err == ERR_DONT_REPLY) {
            continue;
        }
        // If the handler returned an error, reply an error message.
        if (err != OK) {
            m.header = ERROR_TO_HEADER(err);
        }
        // Send the reply message.
        TRY(ipc_send(m.from, &m));
    }
}
