#include <list.h>
#include <cstring.h>
#include <types.h>
#include "ipc.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"

extern uint8_t __temp_page[];

static void resume_sender_task(struct task *task) {
    LIST_FOR_EACH (sender, &task->senders, struct task, sender_next) {
        if (task->src == IPC_ANY || task->src == sender->tid) {
            DEBUG_ASSERT(sender->state == TASK_SENDING);
            task_set_state(sender, TASK_RUNNABLE);
            list_remove(&sender->sender_next);
            break;
        }
    }
}

/// Sends and receives a message. Note that `m` is a user pointer if
/// IPC_KERNEL is not set!
error_t ipc(struct task *dst, task_t src, struct ipc_msg_t *m, unsigned flags) {
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

        // Copy the message into the receiver's buffer.
        if (flags & IPC_KERNEL) {
            memcpy(&dst->m, m, sizeof(struct ipc_msg_t));
        } else {
            memcpy_from_user(&dst->m, (userptr_t) m, sizeof(struct ipc_msg_t));
        }

        // Copy the bulk payload.
        unsigned ptr_offset = MSG_BULK_PTR(dst->m.type) * sizeof(uintptr_t);
        unsigned len_offset = MSG_BULK_LEN(dst->m.type) * sizeof(uintptr_t);
        if (!IS_ERROR(dst->m.type) && ptr_offset) {
            size_t len = *((size_t *) ((uintptr_t) &dst->m + len_offset));
            userptr_t src_buf = *((userptr_t *) ((uintptr_t) &dst->m + ptr_offset));
            userptr_t dst_buf = dst->bulk_ptr;
            if (!dst_buf) {
                resume_sender_task(dst);
                return ERR_NOT_ACCEPTABLE;
            }

            if (len > dst->bulk_len) {
                resume_sender_task(dst);
                return ERR_TOO_LARGE;
            }

            size_t remaining = len;
            while (remaining > 0) {
                size_t copy_len = MIN(MIN(remaining, PAGE_SIZE - (src_buf % PAGE_SIZE)),
                                        PAGE_SIZE - (dst_buf % PAGE_SIZE));
                paddr_t paddr = vm_resolve(&dst->vm, ALIGN_DOWN(dst_buf, PAGE_SIZE));
                DEBUG_ASSERT(paddr);

                vm_link(&CURRENT->vm, (vaddr_t) __temp_page, paddr, PAGE_WRITABLE);
                memcpy_from_user(&__temp_page[dst_buf % PAGE_SIZE], src_buf, copy_len);
                remaining -= copy_len;
                dst_buf += copy_len;
                src_buf += copy_len;
            }

            *((userptr_t *) ((uintptr_t) &dst->m + ptr_offset)) = dst->bulk_ptr;
        }

        dst->m.src = (flags & IPC_KERNEL) ? KERNEL_TASK_TID : CURRENT->tid;
        task_set_state(dst, TASK_RUNNABLE);
    }

    // Receive a message.
    if (flags & IPC_RECV) {
        // Check if there're pending notifications.
        notifications_t pending = CURRENT->notifications;
        if (src == IPC_ANY && pending) {
            m->type = NOTIFICATIONS_MSG;
            m->src = KERNEL_TASK_TID;
            m->notifications.data = pending;
            CURRENT->notifications = 0;
            return OK;
        }

        // Resume a sender task.
        CURRENT->src = src;
        resume_sender_task(CURRENT);

        // Sleep until a sender task resumes this task...
        task_set_state(CURRENT, TASK_RECEIVING);
        task_switch();

        // Received a message. Copy it into the receiver buffer.
        if (flags & IPC_KERNEL) {
            memcpy(m, &CURRENT->m, sizeof(struct ipc_msg_t));
        } else {
            memcpy_to_user((userptr_t) m, &CURRENT->m, sizeof(struct ipc_msg_t));
        }
    }

    return OK;
}

// Notifies notifications to the task.
void notify(struct task *dst, notifications_t notifications) {
    if (dst->state == TASK_RECEIVING && dst->src == IPC_ANY) {
        // Send a NOTIFICATIONS_MSG message immediately.
        dst->m.type = NOTIFICATIONS_MSG;
        dst->m.src = KERNEL_TASK_TID;
        dst->m.notifications.data = dst->notifications | notifications;
        dst->notifications = 0;
        task_set_state(dst, TASK_RUNNABLE);
    } else {
        // The task is not ready for receiving a event message: update the
        // pending notifications instead.
        dst->notifications |= notifications;
    }
}
