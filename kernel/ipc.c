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
            DEBUG_ASSERT(sender->state == TASK_BLOCKED);
            DEBUG_ASSERT(sender->src == IPC_DENY);

            task_resume(sender);
            list_remove(&sender->sender_next);
            break;
        }
    }
}

static void prefetch_pages(userptr_t base, size_t len) {
    if (!len) {
        return;
    }

    userptr_t ptr = base;
    do {
        char tmp;
        CURRENT->prefetching = true;
        memcpy_from_user(&tmp, ptr, sizeof(tmp));
        CURRENT->prefetching = false;
        ptr += PAGE_SIZE;
    } while (ptr < ALIGN_UP(base + len, PAGE_SIZE));
}

/// Sends and receives a message. Note that `m` is a user pointer if
/// IPC_KERNEL is not set!
error_t ipc(struct task *dst, task_t src, struct message *m, unsigned flags) {
    // TODO: Needs refactoring.
    // TODO: IPC fastpath

    if (flags & IPC_RECV && CURRENT->bulk_ptr && !CURRENT->prefetching) {
        prefetch_pages(CURRENT->bulk_ptr, CURRENT->bulk_len);
    }

    // Send a message.
    if (flags & IPC_SEND) {
        // Copy the message into the receiver's buffer.
        struct message tmp_m;
        if (flags & IPC_KERNEL) {
            memcpy(&tmp_m, m, sizeof(struct message));
        } else {
            memcpy_from_user(&tmp_m, (userptr_t) m, sizeof(struct message));
        }

        bool contains_bulk = !IS_ERROR(tmp_m.type) && (tmp_m.type & MSG_BULK);
        if (contains_bulk) {
            prefetch_pages((userptr_t) tmp_m.bulk_ptr, tmp_m.bulk_len);
        }

        // Wait until the destination (receiver) task gets ready for receiving.
        while (true) {
            if (dst->state == TASK_BLOCKED
                && (dst->src == IPC_ANY || dst->src == CURRENT->tid)) {
                break;
            }

            if (flags & IPC_NOBLOCK) {
                return ERR_WOULD_BLOCK;
            }

            // The receiver task is not ready. Sleep until it resumes the
            // current task.
            CURRENT->src = IPC_DENY;
            task_block(CURRENT);
            list_push_back(&dst->senders, &CURRENT->sender_next);
            task_switch();

            if (CURRENT->notifications & NOTIFY_ABORTED) {
                // The receiver task has exited. Abort the system call.
                CURRENT->notifications &= ~NOTIFY_ABORTED;
                return ERR_ABORTED;
            }
        }

        // Copy the bulk payload.
        if (contains_bulk) {
            size_t len = tmp_m.bulk_len;
            userptr_t src_buf = (userptr_t) tmp_m.bulk_ptr;
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

            tmp_m.bulk_ptr = (void *) dst->bulk_ptr;
        }

        tmp_m.src = (flags & IPC_KERNEL) ? KERNEL_TASK_TID : CURRENT->tid;
        memcpy(&dst->m, &tmp_m, sizeof(dst->m));
        task_resume(dst);

#ifdef CONFIG_TRACE_IPC
        if (contains_bulk) {
            TRACE("IPC: %s: %s -> %s (%d bytes in bulk%s)",
                  msgtype2str(tmp_m.type), CURRENT->name, dst->name,
                  tmp_m.bulk_len, (tmp_m.type & MSG_STR) ? ", string" : "");
        } else {
            TRACE("IPC: %s: %s -> %s",
                  msgtype2str(tmp_m.type), CURRENT->name, dst->name);
        }
#endif
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
        task_block(CURRENT);
        task_switch();

        // Received a message. Copy it into the receiver buffer.
        if (flags & IPC_KERNEL) {
            memcpy(m, &CURRENT->m, sizeof(struct message));
        } else {
            memcpy_to_user((userptr_t) m, &CURRENT->m, sizeof(struct message));
        }
    }

    return OK;
}

// Notifies notifications to the task.
void notify(struct task *dst, notifications_t notifications) {
    if (dst->state == TASK_BLOCKED && dst->src == IPC_ANY) {
        // Send a NOTIFICATIONS_MSG message immediately.
        dst->m.type = NOTIFICATIONS_MSG;
        dst->m.src = KERNEL_TASK_TID;
        dst->m.notifications.data = dst->notifications | notifications;
        dst->notifications = 0;
        task_resume(dst);
    } else {
        // The task is not ready for receiving a event message: update the
        // pending notifications instead.
        dst->notifications |= notifications;
    }
}
