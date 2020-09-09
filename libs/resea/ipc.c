#include <resea/ipc.h>
#include <resea/syscall.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

/// The internal buffer to receive ool payloads.
#ifndef CONFIG_NOMMU
static void *ool_ptr = NULL;
static const size_t ool_len = CONFIG_OOL_BUFFER_LEN;
#endif

// for sparse
error_t call_pager(struct message *m);

__weak error_t call_pager(struct message *m) {
#ifdef CONFIG_NOMMU
    // We don't use this feature. Discard messages with a warning.
    WARN("ignoring %s", msgtype2str(m->type));
    return OK;
#else
    return ipc_call(INIT_TASK, m);
#endif
}

#ifndef CONFIG_NOMMU
static void ool_recv(vaddr_t ptr, size_t len) {
    struct message m;
    m.type = OOL_RECV_MSG;
    m.ool_recv.addr = ptr;
    m.ool_recv.len = len;
    error_t err = call_pager(&m);
    ASSERT_OK(err);
    ASSERT(m.type == OOL_RECV_REPLY_MSG);
}

static vaddr_t ool_send(task_t dst, vaddr_t ptr, size_t len) {
    struct message m;
    m.type = OOL_SEND_MSG;
    m.ool_send.dst = dst;
    m.ool_send.addr = ptr;
    m.ool_send.len = len;
    error_t err = call_pager(&m);
    ASSERT_OK(err);
    ASSERT(m.type == OOL_SEND_REPLY_MSG);
    return m.ool_send_reply.id;
}

static vaddr_t ool_verify(task_t src, vaddr_t ptr, size_t len) {
    struct message m;
    m.type = OOL_VERIFY_MSG;
    m.ool_verify.src = src;
    m.ool_verify.id = ptr;
    m.ool_verify.len = len;
    error_t err = call_pager(&m);
    if (err != OK) {
        return 0;
    }

    ASSERT(m.type == OOL_VERIFY_REPLY_MSG);
    return m.ool_verify_reply.received_at;
}
#endif

static void pre_send(task_t dst, struct message *m) {
#ifndef CONFIG_NOMMU
    if (!IS_ERROR(m->type) && m->type & MSG_OOL) {
        if (m->type & MSG_STR) {
            m->ool_len = strlen(m->ool_ptr) + 1;
        }

        m->ool_ptr = (void *) ool_send(dst, (vaddr_t) m->ool_ptr, m->ool_len);
    }
#endif
}

static void pre_recv(void) {
#ifndef CONFIG_NOMMU
    if (!ool_ptr) {
        ool_ptr = malloc(ool_len);
        ool_recv((vaddr_t) ool_ptr, ool_len);
    }
#endif
}

 static error_t post_recv(error_t err, struct message *m) {
#ifndef CONFIG_NOMMU
    if (!IS_ERROR(m->type) && m->type & MSG_OOL) {
        // Received a ool payload.
        m->ool_ptr = (void *) ool_verify(m->src, (vaddr_t) m->ool_ptr,
                                               m->ool_len);
        if (!m->ool_ptr) {
            WARN_DBG("received an invalid oolcopy payload from #%d: %s",
                     m->src, err2str(err));
            m->type = INVALID_MSG;
            return OK;
        }

        // We've consumed `ool_ptr` so set NULL to it
        // and reallocate the receiver buffer later.
        ool_ptr = NULL;

        // A mitigation for a non-terminated (malicious) string payload.
        if (m->type & MSG_STR) {
            char *str = m->ool_ptr;
            str[m->ool_len] = '\0';
        }
    }
#endif

    // Handle the case when m.type is negative: a message represents an error
    // (sent by `ipc_send_err()`).
    return (IS_OK(err) && m->type < 0) ? m->type : err;
 }

error_t ipc_send(task_t dst, struct message *m) {
    void *saved_ool_ptr = m->ool_ptr;
    pre_send(dst, m);
    error_t err = sys_ipc(dst, 0, m, IPC_SEND);
    m->ool_ptr = saved_ool_ptr;
    return err;
}

error_t ipc_send_noblock(task_t dst, struct message *m) {
    void *saved_ool_ptr = m->ool_ptr;
    pre_send(dst, m);
    error_t err = sys_ipc(dst, 0, m, IPC_SEND | IPC_NOBLOCK);
    m->ool_ptr = saved_ool_ptr;
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
    return sys_notify(dst, notifications);
}

error_t ipc_recv(task_t src, struct message *m) {
    pre_recv();
    error_t err = sys_ipc(0, src, m, IPC_RECV);
    return post_recv(err, m);
}

error_t ipc_call(task_t dst, struct message *m) {
    pre_recv();
    pre_send(dst, m);
    error_t err = sys_ipc(dst, dst, m, IPC_CALL);
    return post_recv(err, m);
}

error_t ipc_replyrecv(task_t dst, struct message *m) {
    pre_recv();
    pre_send(dst, m);
    unsigned flags = (dst < 0) ? IPC_RECV : (IPC_SEND | IPC_RECV | IPC_NOBLOCK);
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

/// Discards an unknown message.
void discard_unknown_message(struct message *m) {
    OOPS("received an unknown message (%s [%d]%s)",
         msgtype2str(m->type),
         MSG_ID(m->type),
         (m->type & MSG_OOL) ? ", ool" : "",
         (m->type & MSG_STR) ? ", str" : "");

    if (m->type & MSG_OOL) {
        free(m->ool_ptr);
    }
}
