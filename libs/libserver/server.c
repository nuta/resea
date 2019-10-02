#include <resea.h>
#include <idl_stubs.h>
#include <server.h>

#define DEFERRED_WORK_DELAY      100
#define DEFERRED_WORK_DELAY_MAX  (5 * 1000)

error_t server_mainloop_with_deferred(cid_t ch,
                                      error_t (*process)(struct message *m),
                                      error_t (*deferred_work)(void)) {
    int retries = 0;
    cid_t timer_ch;
    if (deferred_work) {
        TRY_OR_OOPS(open(&timer_ch));
        TRY_OR_OOPS(transfer(timer_ch, ch));
    }

    struct message m;
    while (1) {
        // Receive a message from a client.
        TRY(ipc_recv(ch, &m));

        bool deferred_work_timeout = ((m.notification & NOTIFY_TIMER) != 0);

        // Do work and fill a reply message into `m`.
        error_t err = process(&m);

        // Warn if the handler returned ERR_UNEXPECTED_MESSAGE since it is
        // likely that the handler simply does not yet implement it.
        if (err == ERR_UNEXPECTED_MESSAGE) {
            WARN("unexpected message type: %x", MSG_TYPE(m.header));
        }

        // If the handler returned DONT_REPLY, we don't reply.
        if (err != DONT_REPLY) {
            // If the handler returned an error, reply an error message.
            if (err != OK) {
                m.header = ERROR_TO_HEADER(err);
            }
            // Send the reply message.
            TRY(ipc_send(m.from, &m));
        }

        // Do the deferred work if specified.
        if (deferred_work) {
            if (retries == 0 || deferred_work_timeout) {
                err = deferred_work();
                if (err == ERR_NEEDS_RETRY) {
                    // Retry the deferred work later.
                    // FIXME: Assuming @1 serves the timer interface.
                    cid_t timer_server = 1;
                    retries++;
                    uint32_t delay = MIN(DEFERRED_WORK_DELAY * retries,
                                         DEFERRED_WORK_DELAY_MAX);
                    TRACE("Retrying deferred work in %d milliseconds...",
                          delay);
                    int timer_id;
                    call_timer_set(timer_server, timer_ch, delay, 0, &timer_id);
                } else {
                    TRY(err);
                    retries = 0;
                }
            }
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
