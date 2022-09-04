#include "ipc.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"
#include <list.h>
#include <message.h>
#include <string.h>
#include <types.h>

/// Resumes a sender task for the `receiver` tasks and updates
/// `receiver->waiting_for` properly.
static void resume_sender(struct task *receiver, task_t wait_for) {
    LIST_FOR_EACH(sender, &receiver->senders, struct task, next) {
        if (wait_for == IPC_ANY || wait_for == sender->tid) {
            DEBUG_ASSERT(sender->state == TASK_BLOCKED);
            DEBUG_ASSERT(sender->waiting_for == IPC_DENY);
            task_resume(sender);
            list_remove(&sender->next);

            // If src == IPC_ANY, allow only `sender` to send a message. Let's
            // consider the following situation to understand why:
            //
            //     [Sender A]              [Receiver C]              [Sender B]
            //         .                        |                        |
            // in C's sender queue              |                        |
            //         .                        |                        |
            //         .        Resume          |                        |
            //         + <--------------------- |                        |
            //         .                        .    Try sending (X)     |
            //         .                        + <--------------------- |
            //         .                        |                        |
            //         V                        |                        |
            //         |                        |                        |
            //
            // When (X) occurrs, the receiver should not accept the message
            // from B since C has already resumed A as the next sender.
            //
            receiver->waiting_for = sender->tid;
            return;
        }
    }

    receiver->waiting_for = wait_for;
}

/// Sends and receives a message. Note that `m` is a user pointer if
/// IPC_KERNEL is not set!
static error_t ipc_slowpath(struct task *dst, task_t src,
                            __user struct message *m, unsigned flags) {
    // Send a message.
    if (flags & IPC_SEND) {
        // Copy the message into the receiver's buffer in case the receiver is
        // the current's pager task and accessing `m` cause the page fault. If
        // it happens, it leads to a dead lock.
        struct message tmp_m;
        if (flags & IPC_KERNEL) {
            memcpy(&tmp_m, (const void *) m, sizeof(struct message));
        } else {
            memcpy_from_user(&tmp_m, m, sizeof(struct message));
        }

        // Check whether the destination (receiver) task is ready for receiving.
        bool receiver_is_ready = dst->state == TASK_BLOCKED
                                 && (dst->waiting_for == IPC_ANY
                                     || dst->waiting_for == CURRENT_TASK->tid);
        if (!receiver_is_ready) {
            if (flags & IPC_NOBLOCK) {
                return ERR_WOULD_BLOCK;
            }

            // The receiver task is not ready. Sleep until it resumes the
            // current task.
            CURRENT_TASK->waiting_for = IPC_DENY;
            task_block(CURRENT_TASK);
            list_push_back(&dst->senders, &CURRENT_TASK->next);
            task_switch();

            if (CURRENT_TASK->notifications & NOTIFY_ABORTED) {
                // The receiver task has exited. Abort the system call.
                CURRENT_TASK->notifications &= ~NOTIFY_ABORTED;
                return ERR_ABORTED;
            }
        }

        // We've gone beyond the point of no return. We must not abort the
        // sending from here: don't return an error or cause a page fault!
        //
        // If you need to do so, push CURRENT back into the senders queue.

        // Copy the message.
        tmp_m.src = (flags & IPC_KERNEL) ? KERNEL_TASK : CURRENT_TASK->tid;
        memcpy(&dst->m, &tmp_m, sizeof(dst->m));

        // Resume the receiver task.
        task_resume(dst);

#ifdef CONFIG_TRACE_IPC
        TRACE("IPC: %s: %s -> %s", msgtype2str(tmp_m.type), CURRENT_TASK->name,
              dst->name);
#endif
    }

    // Receive a message.
    if (flags & IPC_RECV) {
        struct message tmp_m;
        if (src == IPC_ANY && CURRENT_TASK->notifications) {
            // Receive pending notifications as a message.
            NYI();
            CURRENT_TASK->notifications = 0;
        } else {
            if ((flags & IPC_NOBLOCK) != 0) {
                return ERR_WOULD_BLOCK;
            }

            // Resume a sender task and sleep until a sender task resumes this
            // task...
            resume_sender(CURRENT_TASK, src);
            task_block(CURRENT_TASK);
            task_switch();

            // Copy into `tmp_m` since memcpy_to_user may cause a page fault and
            // CURRENT_TASK->m will be overwritten by page fault mesages.
            memcpy(&tmp_m, &CURRENT_TASK->m, sizeof(struct message));
        }

        // Received a message. Copy it into the receiver buffer.
        if (flags & IPC_KERNEL) {
            memcpy((void *) m, &tmp_m, sizeof(struct message));
        } else {
            memcpy_to_user(m, &tmp_m, sizeof(struct message));
        }
    }

    return OK;
}

/// The IPC fastpath: an IPC implementation optimized for the common case.
///
/// Note that `m` is a user pointer if IPC_KERNEL is not set!
error_t ipc(struct task *dst, task_t src, __user struct message *m,
            unsigned flags) {
    if (dst == CURRENT_TASK) {
        WARN_DBG("%s: tried to send a message to myself", CURRENT_TASK->name);
        return ERR_INVALID_ARG;
    }

    // THe send phase: copy the message and resume the receiver task. Note
    // that this user copy may cause a page fault.
    memcpy_from_user(&dst->m, m, sizeof(struct message));
    dst->m.src = CURRENT_TASK->tid;
    task_resume(dst);

    // #    ifdef CONFIG_TRACE_IPC
    // INFO("IPC: %s: %s -> %s (fastpath)", msgtype2str(dst->m.type),
    //   CURRENT_TASK->name, dst->name);
    // #    endif

    // The receive phase: wait for a message, copy it into the user's
    // buffer, and return to the user.
    resume_sender(CURRENT_TASK, src);
    task_block(CURRENT_TASK);
    task_switch();

    // This user copy should not cause a page fault since we've filled the
    // page in the user copy above.
    memcpy_to_user(m, &CURRENT_TASK->m, sizeof(struct message));
    return OK;
}

// Notifies notifications to the task.
void notify(struct task *dst, notifications_t notifications) {
    if (dst->state == TASK_BLOCKED && dst->waiting_for == IPC_ANY) {
        // Send a NOTIFICATIONS_MSG message immediately.
        NYI();
        dst->notifications = 0;
    } else {
        // The task is not ready for receiving a event message: update the
        // pending notifications instead.
        dst->notifications |= notifications;
        task_resume(dst);
    }
}
