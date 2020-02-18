#include <list.h>
#include <std/async.h>
#include <std/malloc.h>
#include <std/syscall.h>
#include <string.h>

static list_t pending;

void async_send(tid_t dst, struct message *m, size_t len) {
    error_t err = ipc_send_noblock(dst, m, len);
    // TODO: Should we handle other errors?
    switch (err) {
        case OK:
            return;
        case ERR_WOULD_BLOCK: {
            // The receiver is not ready. We need to enqueue it and try later in
            // `async_flush()`.
            struct message *buf = malloc(len);
            memcpy(buf, m, len);
            struct async_message *am = malloc(sizeof(*am));
            am->dst = dst;
            am->len = len;
            am->m = buf;
            list_push_back(&pending, &am->next);
            ipc_listen(dst);
            break;
        }
    }
}

void async_flush(void) {
    LIST_FOR_EACH (am, &pending, struct async_message, next) {
        error_t err = ipc_send_noblock(am->dst, am->m, am->len);
        // TODO: Should we handle other errors?
        switch (err) {
            case OK:
                // Hooray! Successfully sent the message. Remove it from the
                // list and free memory.
                list_remove(&am->next);
                free(am->m);
                free(am);
                break;
            case ERR_WOULD_BLOCK:
                // The receiver is still not ready. Try again next time.
                break;
        }
    }
}

void async_init(void) {
    list_init(&pending);
}
