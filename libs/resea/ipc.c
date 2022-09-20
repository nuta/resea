#include <print.h>
#include <resea/async_ipc.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/syscall.h>
#include <string.h>

error_t ipc_send(task_t dst, struct message *m) {
    return sys_ipc(dst, 0, m, IPC_SEND);
}

error_t ipc_send_noblock(task_t dst, struct message *m) {
    return sys_ipc(dst, 0, m, IPC_SEND | IPC_NOBLOCK);
}

void ipc_reply(task_t dst, struct message *m) {
    error_t err = ipc_send_noblock(dst, m);
    OOPS_OK(err);
}

void ipc_reply_err(task_t dst, error_t error) {
    struct message m;
    m.type = ERROR_MSG;
    m.error.error = error;
    ipc_reply(dst, &m);
}

static notifications_t pending = 0;
static error_t recv_notification_as_message(struct message *m) {
    // FIXME: ffsll doesn't cover bigger values!
    error_t err;
    int index = __builtin_ffsll(pending) - 1;
    DEBUG_ASSERT(index >= 0);
    switch (index) {
        case NOTIFY_ABORTED:
            m->type = NOTIFY_ABORTED_MSG;
            err = OK;
            break;
        case NOTIFY_ASYNC_OFF ... NOTIFY_ASYNC_OFFEND - 1: {
            task_t src = index - NOTIFY_ASYNC_OFF;
            err = async_recv(src, m);
            break;
        }
        default:
            UNREACHABLE();
    }

    pending &= ~(1 << index);
    return err;
}

error_t ipc_recv(task_t src, struct message *m) {
retry:
    if (pending) {
        return recv_notification_as_message(m);
    }

    error_t err = sys_ipc(0, src, m, IPC_RECV);
    if (err != OK) {
        return err;
    }

    switch (m->type) {
        case ERROR_MSG:
            return m->error.error;
        case NOTIFY_MSG:
            if (m->src != KERNEL_TASK_ID) {
                WARN_DBG("received a notification from a non-kernel task %d",
                         m->src);
                return ERR_TRY_AGAIN;
            }

            pending |= m->notify.notifications;
            return recv_notification_as_message(m);
        case ASYNC_RECV_MSG:
            if (src == IPC_ANY) {
                error_t err = async_reply(m->src);
                if (err != OK) {
                    WARN_DBG("failed to send a async message to %d: %s", m->src,
                             err2str(err));
                }
            }
            goto retry;
    }

    return OK;
}

error_t ipc_call(task_t dst, struct message *m) {
    error_t err = sys_ipc(dst, dst, m, IPC_CALL);
    return (IS_OK(err) && m->type == ERROR_MSG) ? m->error.error : err;
}

error_t ipc_notify(task_t dst, notifications_t notifications) {
    return sys_notify(dst, notifications);
}
