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
        return ipc_call(INIT_TASK, m);
    }
#endif
}

static void accept_bulkcopy(vaddr_t ptr, size_t len) {
    struct message m;
    m.type = ACCEPT_BULKCOPY_MSG;
    m.accept_bulkcopy.addr = ptr;
    m.accept_bulkcopy.len = len;
    error_t err = call_pager(&m);
    ASSERT_OK(err);
    ASSERT(m.type == ACCEPT_BULKCOPY_REPLY_MSG);
}

static vaddr_t do_bulkcopy(task_t dst, vaddr_t ptr, size_t len) {
    struct message m;
    m.type = DO_BULKCOPY_MSG;
    m.do_bulkcopy.dst = dst;
    m.do_bulkcopy.addr = ptr;
    m.do_bulkcopy.len = len;
    error_t err = call_pager(&m);
    ASSERT_OK(err);
    ASSERT(m.type == DO_BULKCOPY_REPLY_MSG);
    return m.do_bulkcopy_reply.id;
}

static vaddr_t verify_bulkcopy(task_t src, vaddr_t ptr, size_t len) {
    struct message m;
    m.type = VERIFY_BULKCOPY_MSG;
    m.verify_bulkcopy.src = src;
    m.verify_bulkcopy.id = ptr;
    m.verify_bulkcopy.len = len;
    error_t err = call_pager(&m);
    if (err != OK) {
        return 0;
    }

    ASSERT(m.type == VERIFY_BULKCOPY_REPLY_MSG);
    return m.verify_bulkcopy_reply.received_at;
}

static void pre_send(task_t dst, struct message *m) {
#ifndef CONFIG_NOMMU
    if (!IS_ERROR(m->type) && m->type & MSG_BULK) {
        if (m->type & MSG_STR) {
            m->bulk_len = strlen(m->bulk_ptr) + 1;
        }

        m->bulk_ptr = (void *) do_bulkcopy(dst, (vaddr_t) m->bulk_ptr, m->bulk_len);
    }
#endif
}

static void pre_recv(void) {
#ifndef CONFIG_NOMMU
    if (!bulk_ptr) {
        bulk_ptr = malloc(bulk_len);
        accept_bulkcopy((vaddr_t) bulk_ptr, bulk_len);
    }
#endif
}

 static error_t post_recv(error_t err, struct message *m) {
#ifndef CONFIG_NOMMU
    if (!IS_ERROR(m->type) && m->type & MSG_BULK) {
        // Received a bulk payload.
        m->bulk_ptr = (void *) verify_bulkcopy(m->src, (vaddr_t) m->bulk_ptr,
                                               m->bulk_len);
        if (!m->bulk_ptr) {
            WARN_DBG("received an invalid bulkcopy payload from #%d: %s",
                     m->src, err2str(err));
            m->type = INVALID_MSG;
            return OK;
        }

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
    void *saved_bulk_ptr = m->bulk_ptr;
    pre_send(dst, m);
    error_t err = sys_ipc(dst, 0, m, IPC_SEND);
    m->bulk_ptr = saved_bulk_ptr;
    return err;
}

error_t ipc_send_noblock(task_t dst, struct message *m) {
    void *saved_bulk_ptr = m->bulk_ptr;
    pre_send(dst, m);
    error_t err = sys_ipc(dst, 0, m, IPC_SEND | IPC_NOBLOCK);
    m->bulk_ptr = saved_bulk_ptr;
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
