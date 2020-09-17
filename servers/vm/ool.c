#include <resea/ipc.h>
#include <resea/task.h>
#include <string.h>
#include <message.h>
#include "ool.h"
#include "mm.h"
#include "task.h"

static uint8_t __src_page[PAGE_SIZE] __aligned(PAGE_SIZE);
static uint8_t __dst_page[PAGE_SIZE] __aligned(PAGE_SIZE);

error_t handle_ool_recv(struct message *m) {
    struct task *task = task_lookup(m->src);
    ASSERT(task);

//    TRACE("accept: %s: %p %d (old=%p)",
//        task->name, m->ool_recv.addr, m->ool_recv.len, task->ool_buf);
    if (task->ool_buf) {
        return ERR_ALREADY_EXISTS;
    }

    task->ool_buf = m->ool_recv.addr;
    task->ool_len = m->ool_recv.len;

    struct task *sender = LIST_POP_FRONT(&task->ool_sender_queue, struct task,
                                         ool_sender_next);
    if (sender) {
        struct message m;
        memcpy(&m, &sender->ool_sender_m, sizeof(m));
//        TRACE("%s -> %s: src = %d / %d", task->name, sender->name,
//              sender->ool_sender_m.src, m.src);
        error_t err = handle_ool_send(&m);
        switch (err) {
            case OK:
                ipc_reply(sender->tid, &m);
                break;
            case DONT_REPLY:
                // Do nothing.
                break;
            default:
                OOPS_OK(err);
                ipc_reply_err(sender->tid, err);
        }
    }

    m->type = OOL_RECV_REPLY_MSG;
    return OK;
}

error_t handle_ool_verify(struct message *m) {
    struct task *task = task_lookup(m->src);
    ASSERT(task);

//    TRACE("verify: %s: id=%p len=%d (src=%d)", task->name,
//          m->ool_verify.id, m->ool_verify.len, m->src);
    if (m->ool_verify.src != task->received_ool_from
        || m->ool_verify.id != task->received_ool_buf
        || m->ool_verify.len != task->received_ool_len) {
        return ERR_INVALID_ARG;
    }

    m->type = OOL_VERIFY_REPLY_MSG;
    m->ool_verify_reply.received_at = task->received_ool_buf;

    task->received_ool_buf = 0;
    task->received_ool_len = 0;
    task->received_ool_from = 0;
    return OK;
}

error_t handle_ool_send(struct message *m) {
    struct task *src_task = task_lookup(m->src);
    ASSERT(src_task);

    struct task *dst_task = task_lookup(m->ool_send.dst);
    if (!dst_task) {
        return ERR_NOT_FOUND;
    }

//    TRACE("do_copy: %s -> %s: %p -> %p, len=%d",
//        src_task->name, dst_task->name,
//        m->ool_send.addr, dst_task->ool_buf,
//        m->ool_send.len);
    if (!dst_task->ool_buf) {
        memcpy(&src_task->ool_sender_m, m, sizeof(*m));
        list_push_back(&dst_task->ool_sender_queue, &src_task->ool_sender_next);
        return DONT_REPLY;
    }

    size_t len = m->ool_send.len;
    vaddr_t src_buf = m->ool_send.addr;
    vaddr_t dst_buf = dst_task->ool_buf;
    DEBUG_ASSERT(len <= dst_task->ool_len);

    size_t remaining = len;
    while (remaining > 0) {
        offset_t src_off = src_buf % PAGE_SIZE;
        offset_t dst_off = dst_buf % PAGE_SIZE;
        size_t copy_len = MIN(remaining, MIN(PAGE_SIZE - src_off, PAGE_SIZE - dst_off));

        void *src_ptr;
        if (src_task == vm_task) {
            src_ptr = (void *) src_buf;
        } else {
            paddr_t src_paddr = vaddr2paddr(src_task, ALIGN_DOWN(src_buf, PAGE_SIZE));
            if (!src_paddr) {
                task_kill(src_task);
                return DONT_REPLY;
            }

            ASSERT_OK(map_page(vm_task, (vaddr_t) __src_page, src_paddr,
                               MAP_W, true));
            src_ptr = &__src_page[src_off];
        }

        void *dst_ptr;
        if (dst_task == vm_task) {
            dst_ptr = (void *) dst_buf;
        } else {
            paddr_t dst_paddr = vaddr2paddr(dst_task, ALIGN_DOWN(dst_buf, PAGE_SIZE));
            if (!dst_paddr) {
                task_kill(dst_task);
                return ERR_UNAVAILABLE;
            }

            // Temporarily map the pages into the our address space.
            ASSERT_OK(map_page(vm_task, (vaddr_t) __dst_page, dst_paddr,
                               MAP_W, true));
            dst_ptr = &__dst_page[dst_off];
        }

        // Copy between the tasks.
        memcpy(dst_ptr, src_ptr, copy_len);
        remaining -= copy_len;
        dst_buf += copy_len;
        src_buf += copy_len;
    }

    dst_task->received_ool_buf = dst_task->ool_buf;
    dst_task->received_ool_len = m->ool_send.len;
    dst_task->received_ool_from = src_task->tid;
    dst_task->ool_buf = 0;
    dst_task->ool_len = 0;

    m->type = OOL_SEND_REPLY_MSG;
    m->ool_send_reply.id = dst_task->received_ool_buf;
    return OK;
}
