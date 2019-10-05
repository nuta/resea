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
        while (true) {
            error_t err = ipc_recv(ch, &m);
            if (err == OK) {
                break;
            }

            if (err == ERR_NEEDS_RETRY) {
                continue;
            }

            // An unexpected error.
            OOPS("unexpected error (%vE) from ipc_recv", err);
            return err;
        }

        bool deferred_work_timeout = ((m.notification & NOTIFY_TIMER) != 0);

        // Do work and fill a reply message into `m`.
        error_t err = process(&m);

        // Warn if the handler returned ERR_UNEXPECTED_MESSAGE since it is
        // likely that the handler simply does not yet implement it.
        if (err == ERR_UNEXPECTED_MESSAGE) {
            WARN("unexpected message type: %x", m.header);
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
                    // FIXME: We can't reuse timer_ch!
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

error_t server_register(cid_t discovery_server, cid_t server_ch,
                        uint16_t interface) {
    cid_t ch;
    TRY(open(&ch));
    TRY(transfer(ch, server_ch));
    TRY(call_discovery_publicize(discovery_server, interface, ch));
    return OK;
}

error_t server_connect(cid_t discovery_server, uint16_t interface, cid_t *ch) {
    TRY(call_discovery_connect(discovery_server, interface, ch));
    return OK;
}
