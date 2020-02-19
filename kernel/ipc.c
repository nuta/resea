#include <list.h>
#include <string.h>
#include <types.h>
#include "ipc.h"
#include "printk.h"
#include "task.h"

static error_t send(struct task *dst, tid_t send_as, uint32_t flags) {
    // Wait until the destination (receiver) task gets ready for receiving.
    while (true) {
        if (dst->state == TASK_RECEIVING
            && (dst->src == IPC_ANY || dst->src == CURRENT->tid)) {
            break;
        }

        if (flags & IPC_NOBLOCK) {
            return ERR_WOULD_BLOCK;
        }

        // The receiver task is not ready. Sleep until it resumes the
        // current task.
        task_set_state(CURRENT, TASK_SENDING);
        list_push_back(&dst->senders, &CURRENT->sender_next);
        task_switch();

        if (CURRENT->notifications & NOTIFY_ABORTED) {
            // The receiver task has exited. Abort the system call.
            CURRENT->notifications &= ~NOTIFY_ABORTED;
            return ERR_ABORTED;
        }
    }

    // Copy the message into the receiver's buffer and resume it.
    memcpy(dst->buffer, CURRENT->buffer, IPC_SEND_LEN(flags));
    dst->buffer->src = send_as;
    dst->buffer->len = IPC_SEND_LEN(flags);
    task_set_state(dst, TASK_RUNNABLE);
    return OK;
}

static error_t recv(tid_t src) {
    // Check if there're pending notifications.
    if (src == IPC_ANY && CURRENT->notifications) {
        // Receive the pending notifications.
        CURRENT->buffer->type = NOTIFICATIONS_MSG;
        CURRENT->buffer->src = KERNEL_TASK_TID;
        CURRENT->buffer->len = sizeof(struct message);
        CURRENT->buffer->notifications.data = CURRENT->notifications;
        CURRENT->notifications = 0;
        return OK;
    }

    // Resume a sender task.
    LIST_FOR_EACH (sender, &CURRENT->senders, struct task, sender_next) {
        if (src == IPC_ANY || src == sender->tid) {
            task_set_state(sender, TASK_RUNNABLE);
            list_remove(&sender->sender_next);
            break;
        }
    }

    // Notify the listeners that this task is now waiting for a message.
    for (unsigned i = 0; i < TASKS_MAX; i++) {
        if (CURRENT->listened_by[i]) {
            notify(task_lookup(i + 1), NOTIFY_READY);
            CURRENT->listened_by[i] = false;
        }
    }

    // Sleep until a sender task resumes this task...
    CURRENT->src = src;
    task_set_state(CURRENT, TASK_RECEIVING);
    task_switch();
    return OK;
}

/// Sends and/or receives a message. The send length in `flags` is already
/// checked by the caller.
error_t ipc(struct task *dst, tid_t src, tid_t send_as, uint32_t flags) {
    if (flags & IPC_TIMER) {
        CURRENT->timeout = POW2(IPC_TIMEOUT(flags));
    }

    // Register the current task as a listener.
    if (flags & IPC_LISTEN) {
        dst->listened_by[CURRENT->tid - 1] = true;
        return OK;
    }

    // Send a message.
    if (flags & IPC_SEND) {
        error_t err = send(dst, send_as, flags);
        if (IS_ERROR(err)) {
            return err;
        }
    }

    // Receive a message.
    if (flags & IPC_RECV) {
        error_t err = recv(src);
        if (IS_ERROR(err)) {
            return err;
        }
    }

    return OK;
}

/// Sends and/or receives a message from the kernel space. The size of message
/// is fixed to `sizeof(struct message)`.
error_t kernel_ipc(struct task *dst, tid_t src, struct message *m,
                   uint32_t ops) {
    struct message *saved_buffer = CURRENT->buffer;
    uint32_t flags = IPC_FLAGS(ops, sizeof(*m), sizeof(*m), 0);

    // Temporarily switch the buffer in order not to overwrite CURRENT->buffer
    // when we handle a page fault occurred during copying a received message
    // in `sys_ipc()`.
    CURRENT->buffer = m;
    error_t err = ipc(dst, src, KERNEL_TASK_TID, flags);
    CURRENT->buffer = saved_buffer;
    return err;
}

// Notifies notifications to the task.
void notify(struct task *dst, notifications_t notifications) {
    if (dst->state == TASK_RECEIVING && dst->src == IPC_ANY) {
        // Send a NOTIFICATIONS_MSG message immediately.
        dst->buffer->type = NOTIFICATIONS_MSG;
        dst->buffer->src = 0; /* kernel */
        dst->buffer->len = sizeof(struct message);
        dst->buffer->notifications.data = dst->notifications | notifications;
        dst->notifications = 0;
        task_set_state(dst, TASK_RUNNABLE);
    } else {
        // The task is not ready for receiving a event message: update the
        // pending notifications instead.
        dst->notifications |= notifications;
    }
}
