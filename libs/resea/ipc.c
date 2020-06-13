#include <resea/ipc.h>
#include <resea/syscall.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

/// The internal buffer to receive bulk payloads.
static void *bulk_ptr = NULL;
static const size_t bulk_len = CONFIG_BULK_BUFFER_LEN;

static unsigned pre_send(struct message *m) {
    if (m->type & MSG_STR) {
        m->bulk_len = strlen(m->bulk_ptr) + 1;
    }

    unsigned flags = 0;
    if (m->type & MSG_BULK) {
        flags |= IPC_BULK;
    }

    return flags;
 }

error_t ipc_send(task_t dst, struct message *m) {
    unsigned flags = pre_send(m);
    return sys_ipc(dst, 0, m, flags | IPC_SEND);
}

error_t ipc_send_noblock(task_t dst, struct message *m) {
    unsigned flags = pre_send(m);
    return sys_ipc(dst, 0, m, flags | IPC_SEND | IPC_NOBLOCK);
}

error_t ipc_send_err(task_t dst, error_t error) {
    struct message m;
    m.type = error;
    return ipc_send(dst, &m);
}

void ipc_reply(task_t dst, struct message *m) {
    ipc_send_noblock(dst, m);
}

void ipc_reply_err(task_t dst, error_t error) {
    struct message m;
    m.type = error;
    ipc_send_noblock(dst, &m);
}

error_t ipc_notify(task_t dst, notifications_t notifications) {
    return sys_ipc(dst, 0, (void *) (uintptr_t) notifications, IPC_NOTIFY);
}

error_t ipc_recv(task_t src, struct message *m) {
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);
        ASSERT_OK(sys_setattrs(bulk_ptr, bulk_len, 0));
    }

    error_t err = sys_ipc(0, src, m, IPC_RECV);

    if (m->type & MSG_BULK) {
        // Received a bulk payload. We've consumed `bulk_ptr` so set NULL to it
        // and reallocate the receiver buffer later.
        bulk_ptr = NULL;

        // A mitigation for a non-terminated (malicious) string payload.
        if (m->type & MSG_STR) {
            char *str = m->bulk_ptr;
            str[m->bulk_len] = '\0';
        }
    }


    // Handle the case when m.type is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}

error_t ipc_call(task_t dst, struct message *m) {
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);
        ASSERT_OK(sys_setattrs(bulk_ptr, bulk_len, 0));
    }

    unsigned flags = pre_send(m);
    error_t err = sys_ipc(dst, dst, m, flags | IPC_CALL);

    if (m->type & MSG_BULK) {
        bulk_ptr = NULL;
    }

    // Handle the case when `r->type` is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}

error_t ipc_replyrecv(task_t dst, struct message *m) {
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);
        ASSERT_OK(sys_setattrs(bulk_ptr, bulk_len, 0));
    }

    unsigned flags = pre_send(m);
    if (dst < 0) {
        flags |= IPC_RECV;
    } else {
        flags |= IPC_SEND | IPC_RECV | IPC_NOBLOCK;
    }

    error_t err = sys_ipc(dst, IPC_ANY, m, flags);

    if (m->type & MSG_BULK) {
        bulk_ptr = NULL;
    }

    // Handle the case when `r->type` is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
}

task_t ipc_lookup(const char *name) {
    struct message m;
    m.type = LOOKUP_MSG;
    m.lookup.name = name;

    error_t err = ipc_call(INIT_TASK_TID, &m);
    if (IS_ERROR(err)) {
        return err;
    }

    return m.lookup_reply.task;
}
