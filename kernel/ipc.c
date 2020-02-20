#include <list.h>
#include <string.h>
#include <types.h>
#include "ipc.h"
#include "printk.h"
#include "task.h"

/// Sends and receives a message.
error_t ipc(struct task *dst, tid_t src, struct message *m, unsigned flags) {
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
        memcpy(&dst->buffer, m, sizeof(struct message));
        dst->buffer.src = (flags & IPC_KERNEL) ? KERNEL_TASK_TID : CURRENT->tid;
        task_set_state(dst, TASK_RUNNABLE);
    }

    // Receive a message.
    if (flags & IPC_RECV) {
        // Check if there're pending notifications.
        if (src == IPC_ANY && CURRENT->notifications) {
            // Receive the pending notifications.
            m->type = NOTIFICATIONS_MSG;
            m->src = KERNEL_TASK_TID;
            m->notifications.data = CURRENT->notifications;
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

        // Received a message. Copy it into the receiver buffer and return.
        memcpy(m, &CURRENT->buffer, sizeof(struct message));
    }

    return OK;
}

// Notifies notifications to the task.
void notify(struct task *dst, notifications_t notifications) {
    if (dst->state == TASK_RECEIVING && dst->src == IPC_ANY) {
        // Send a NOTIFICATIONS_MSG message immediately.
        dst->buffer.type = NOTIFICATIONS_MSG;
        dst->buffer.src = KERNEL_TASK_TID;
        dst->buffer.notifications.data = dst->notifications | notifications;
        dst->notifications = 0;
        task_set_state(dst, TASK_RUNNABLE);
    } else {
        // The task is not ready for receiving a event message: update the
        // pending notifications instead.
        dst->notifications |= notifications;
    }
}
