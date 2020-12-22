#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/ipc.h>
#include <string.h>

#define NUM_BUCKETS 32

/// A hash table of async message queues.
static list_t *queues[NUM_BUCKETS];

static list_t *get_queue(task_t dst) {
    unsigned index = dst % NUM_BUCKETS;
    list_t *q = queues[index];
    if (!q) {
        q = malloc(sizeof(*q));
        list_init(q);
        queues[index] = q;
    }

    return q;
}

error_t async_send(task_t dst, struct message *m) {
    list_t *q = get_queue(dst);
    struct async_message *am = malloc(sizeof(*am));
    am->dst = dst;
    memcpy(&am->m, m, sizeof(am->m));
    list_nullify(&am->next);
    list_push_back(q, &am->next);

    // Notify the destination task that a new async message is available.
    return ipc_notify(dst, NOTIFY_ASYNC);
}

error_t async_recv(task_t src, struct message *m) {
    m->type = ASYNC_MSG;
    return ipc_call(src, m);
}

error_t async_reply(task_t dst) {
    bool sent = false;
    LIST_FOR_EACH (am, get_queue(dst), struct async_message, next) {
        if (am->dst == dst) {
            if (sent) {
                // Notify that we have more messages for `dst`.
                ipc_notify(dst, NOTIFY_ASYNC);
            } else {
                ipc_reply(dst, &am->m);
                list_remove(&am->next);
                free(am);
                sent = true;
            }
        }
    }

    // Return ER_NOT_FOUND if there're no messages asynchronously sent to `dst`
    // in the queue.
    return (sent) ? OK : ERR_NOT_FOUND;
}

bool async_is_empty(task_t dst) {
    LIST_FOR_EACH (am, get_queue(dst), struct async_message, next) {
        if (am->dst == dst) {
            return false;
        }
    }

    return true;
}
