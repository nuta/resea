#include <resea/ipc.h>
#include <resea/syscall.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

/// The internal buffer to receive bulk payloads.
#ifndef CONFIG_NOMMU
static void *bulk_ptr = NULL;
static const size_t bulk_len = CONFIG_BULK_BUFFER_LEN;
#endif

bool __is_bootstrap(void);

__weak error_t call_self(struct message *m) {
    return OK;
}

static error_t call_pager(struct message *m) {
#ifdef CONFIG_NOMMU
    // We don't use this feature. Discard messages with a warning.
    WARN("ignoring %s", msgtype2str(m->type));
    return OK;
#else
    if (__is_bootstrap()) {
        return call_self(m);
    } else {
        return ipc_call(INIT_TASK_TID, m);
    }
#endif
}

static unsigned pre_send(task_t dst, struct message *m, void **saved_bulk_ptr) {
    *saved_bulk_ptr = m->bulk_ptr;
    unsigned flags = 0;

#ifndef CONFIG_NOMMU
    if (!IS_ERROR(m->type) && m->type & MSG_BULK) {
        flags |= IPC_BULK;
        if (m->type & MSG_STR) {
            m->bulk_len = strlen(m->bulk_ptr) + 1;
        }

        struct message m2;
        m2.type = DO_BULKCOPY_MSG;
        m2.do_bulkcopy.dst = dst;
        m2.do_bulkcopy.addr = (vaddr_t) m->bulk_ptr;
        m2.do_bulkcopy.len = m->bulk_len;
        error_t err = call_pager(&m2);
        OOPS_OK(err);
        ASSERT(m2.type == DO_BULKCOPY_REPLY_MSG);

        m->bulk_ptr = (void *) m2.do_bulkcopy_reply.id;
    }
#endif

    return flags;
 }

static void post_send_only(struct message *m, void **saved_bulk_ptr) {
    m->bulk_ptr = *saved_bulk_ptr;
}

 static void pre_recv(void) {
#ifndef CONFIG_NOMMU
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);

        struct message m;
        m.type = ACCEPT_BULKCOPY_MSG;
        m.accept_bulkcopy.addr = (vaddr_t) bulk_ptr;
        m.accept_bulkcopy.len = bulk_len;
        error_t err = call_pager(&m);
        ASSERT_OK(err);
        ASSERT(m.type == ACCEPT_BULKCOPY_REPLY_MSG);
    }
#endif
}

 static error_t post_recv(error_t err, struct message *m) {
#ifndef CONFIG_NOMMU
    if (!IS_ERROR(m->type) && m->type & MSG_BULK) {

        // Received a bulk payload.
        // TODO: add comment
        struct message m2;
        m2.type = VERIFY_BULKCOPY_MSG;
        m2.verify_bulkcopy.src = m->src;
        m2.verify_bulkcopy.id = (vaddr_t) m->bulk_ptr;
        m2.verify_bulkcopy.len = m->bulk_len;
        error_t err = call_pager(&m2);
        OOPS_OK(err); // FIXME:
        ASSERT(m2.type == VERIFY_BULKCOPY_REPLY_MSG);
        if (err != OK) {
            // FIXME: Use internal original error value to retry.
            return ERR_TRY_AGAIN;
        }

        m->bulk_ptr = (void *) m2.verify_bulkcopy_reply.received_at;

        // We've consumed `bulk_ptr` so set NULL to it
        // and reallocate the receiver buffer later.
        bulk_ptr = NULL;

        // A mitigation for a non-terminated (malicious) string payload.
        if (m->type & MSG_STR) {
            char *str = m->bulk_ptr;
            str[m->bulk_len] = '\0';
        }
    }
#endif

    // Handle the case when m.type is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
 }

error_t ipc_send(task_t dst, struct message *m) {
    void *saved_bulk_ptr;
    unsigned flags = pre_send(dst, m, &saved_bulk_ptr);
    error_t err = sys_ipc(dst, 0, m, flags | IPC_SEND);
    post_send_only(m, &saved_bulk_ptr);
    return err;
}

error_t ipc_send_noblock(task_t dst, struct message *m) {
    void *saved_bulk_ptr;
    unsigned flags = pre_send(dst, m, &saved_bulk_ptr);
    error_t err = sys_ipc(dst, 0, m, flags | IPC_SEND | IPC_NOBLOCK);
    post_send_only(m, &saved_bulk_ptr);
    return err;
}

error_t ipc_send_err(task_t dst, error_t error) {
    struct message m;
    m.type = error;
    return ipc_send(dst, &m);
}

void ipc_reply(task_t dst, struct message *m) {
    error_t err = ipc_send_noblock(dst, m);
    OOPS_OK(err);
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
    pre_recv();
    error_t err = sys_ipc(0, src, m, IPC_RECV);
    return post_recv(err, m);
}

error_t ipc_call(task_t dst, struct message *m) {
    void *saved_bulk_ptr;
    pre_recv();
    unsigned flags = pre_send(dst, m, &saved_bulk_ptr);
    error_t err = sys_ipc(dst, dst, m, flags | IPC_CALL);
    return post_recv(err, m);
}

error_t ipc_replyrecv(task_t dst, struct message *m) {
    void *saved_bulk_ptr;
    pre_recv();
    unsigned flags = pre_send(dst, m, &saved_bulk_ptr);
    flags |= (dst < 0) ? IPC_RECV : (IPC_SEND | IPC_RECV | IPC_NOBLOCK);
    error_t err = sys_ipc(dst, IPC_ANY, m, flags);
    return post_recv(err, m);
}

error_t ipc_serve(const char *name) {
    struct message m;
    m.type = SERVE_MSG;
    m.serve.name = name;
    return call_pager(&m);
}

task_t ipc_lookup(const char *name) {
    struct message m;
    m.type = LOOKUP_MSG;
    m.lookup.name = name;

    error_t err = call_pager(&m);
    if (IS_ERROR(err)) {
        return err;
    }

    ASSERT_OK(m.type == LOOKUP_REPLY_MSG);
    return m.lookup_reply.task;
}
