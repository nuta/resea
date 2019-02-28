#include <ipc.h>
#include <process.h>
#include <thread.h>
#include <server.h>
#include <printk.h>

struct channel *channel_create(struct process *process) {
    int cid = idtable_alloc(&process->channels);
    if (!cid) {
        DEBUG("failed to allocate cid");
        return NULL;
    }

    struct channel *channel = alloc_object();
    if (!channel) {
        DEBUG("failed to allocate a channel");
        idtable_free(&process->channels, cid);
        return NULL;
    }

    channel->cid = cid;
    channel->ref_count = 1;
    channel->process = process;
    channel->linked_to = channel;
    channel->transfer_to = channel;
    channel->receiver = NULL;
    spin_lock_init(&channel->lock);
    list_init(&channel->sender_queue);

    idtable_set(&process->channels, cid, channel);
    return channel;
}

void channel_destroy(UNUSED struct channel *ch) {
    // TODO:
}

void channel_link(struct channel *ch1, struct channel *ch2) {
    if (ch1 == ch2) {
        PANIC("FIXME: link to itself");
    }

    flags_t flags = spin_lock_irqsave(&ch1->lock);
    spin_lock(&ch2->lock);

    ch1->linked_to = ch2;
    ch2->linked_to = ch1;
    ch1->ref_count++;
    ch2->ref_count++;

    spin_unlock(&ch2->lock);
    spin_unlock_irqrestore(&ch1->lock, flags);
}

void channel_transfer(struct channel *src, struct channel *dst) {
    if (src == dst) {
        PANIC("FIXME: trasnfer to itself");
    }

    flags_t flags = spin_lock_irqsave(&src->lock);
    spin_lock(&dst->lock);

    src->transfer_to = dst;
    dst->ref_count++;

    spin_unlock(&dst->lock);
    spin_unlock_irqrestore(&src->lock, flags);
}

cid_t open_handler(void) {
    struct channel *ch = channel_create(CURRENT->process);
    if (!ch) {
        BUG("oh my: %s:%d", __FILE__, __LINE__);
        return ERR_OUT_OF_RESOURCE;
    }

    DEBUG("open: @%d", ch->cid);
    return ch->cid;
}

error_t link_handler(cid_t ch1, cid_t ch2) {
    struct channel *ch1_ch = idtable_get(&CURRENT->process->channels, ch1);
    if (!ch1_ch) {
        BUG("oh my: %s:%d", __FILE__, __LINE__);
        return ERR_INVALID_CID;
    }

    struct channel *ch2_ch = idtable_get(&CURRENT->process->channels, ch2);
    if (!ch2_ch) {
        BUG("oh my: %s:%d", __FILE__, __LINE__);
        return ERR_INVALID_CID;
    }

    DEBUG("link: @%d->@%d", ch1_ch->cid, ch2_ch->cid);
    channel_link(ch1_ch, ch2_ch);
    return OK;
}

error_t transfer_handler(cid_t src, cid_t dst) {
    struct channel *src_ch = idtable_get(&CURRENT->process->channels, src);
    if (!src_ch) {
        BUG("oh my: %s:%d", __FILE__, __LINE__);
        return ERR_INVALID_CID;
    }

    struct channel *dst_ch = idtable_get(&CURRENT->process->channels, dst);
    if (!dst_ch) {
        BUG("oh my: %s:%d", __FILE__, __LINE__);
        return ERR_INVALID_CID;
    }

    DEBUG("trasnfer: @%d->@%d", src_ch->cid, dst_ch->cid);
    channel_transfer(src_ch, dst_ch);
    return OK;
}

#define PAGER_INTERFACE 3
static void copy_payload(struct thread *thread, int index, payload_t header, payload_t p) {
    switch (PAYLOAD_TYPE(header, index)) {
        case INLINE_PAYLOAD:
            thread->buf[index] = p;
            break;
        case PAGE_PAYLOAD: {
            /* FIXME:
            if (thread->process != kernel_process && INTERFACE_ID(header) != PAGER_INTERFACE) {
                PANIC("NYI");
            }
            */

            vaddr_t vaddr = ALIGN_DOWN(p, PAGE_SIZE);
            paddr_t paddr = arch_resolve_paddr_from_vaddr(
                &CURRENT->process->page_table, vaddr);

            if (!paddr) {
                // TODO: return error
                PANIC("NYI");
            }

            thread->buf[index] = paddr;
            break;
        }
    }
}

error_t ipc_handler(payload_t header,
                    payload_t m0,
                    payload_t m1,
                    payload_t m2,
                    payload_t m3,
                    payload_t m4) {

    int cid = header >> 48;
    struct channel *ch = idtable_get(&CURRENT->process->channels, cid);
    if (!ch) {
        return ERR_INVALID_CID;
    }

    if (ch->linked_to->process == kernel_process) {
        if (!FLAG_SEND(header) || !FLAG_RECV(header)) {
            DEBUG("invalid header");
            return ERR_INVALID_HEADER;
        }

        error_t err = kernel_server(header, m0, m1, m2, m3, m4, CURRENT);
        __asm__ __volatile__(
            // "movq %%r, %%rdi   \n"
            "movq 0(%%rax), %%rsi   \n"
            "movq 8(%%rax), %%rdx   \n"
            "movq 16(%%rax), %%r10   \n"
            "movq 24(%%rax), %%r8    \n"
            "movq 32(%%rax), %%r9    \n"
        :: "D"(CURRENT->header), "a"(CURRENT->buf)
        );
        return err;
    }

    if (FLAG_SEND(header)) {
retry_send:;
        struct channel *linked_to = ch->linked_to;
        struct channel *dst = linked_to->transfer_to;

        if (INTERFACE_ID(header) != PUTCHAR_INTERFACE) {
            DEBUG_USER("ipc: @%s.%d -> @%s.%d, m = %s",
                CURRENT->process->name, cid, dst->process->name, dst->cid,
                find_msg_name(MSG_ID(header)));
        }

        flags_t flags = spin_lock_irqsave(&dst->lock);
        struct thread *receiver = dst->receiver;

        if (!receiver) {
            // The receiver is not ready or another thread is sending a message
            // to the channel.
            list_push_back(&dst->sender_queue, &CURRENT->sender_queue_elem);
            thread_block(CURRENT);
            spin_unlock_irqrestore(&dst->lock, flags);
            thread_switch();
            goto retry_send;
        }

        // Now we have the linked_to channel lock and the receiver. Time to send
        // a message!
        cid_t sent_from = linked_to->cid;
        receiver->header = (header & 0x3ffffff) | (sent_from << 48);
        copy_payload(receiver, 0, header, m0);
        copy_payload(receiver, 1, header, m1);
        copy_payload(receiver, 2, header, m2);
        copy_payload(receiver, 3, header, m3);
        copy_payload(receiver, 4, header, m4);

        thread_resume(receiver);
        dst->receiver = NULL;
        spin_unlock_irqrestore(&dst->lock, flags);
    }

    if (FLAG_RECV(header)) {
        struct channel *recv_on;
        if (FLAG_REPLY(header)) {
            recv_on = ch->transfer_to;
        } else {
            recv_on = ch;
        }

        flags_t flags = spin_lock_irqsave(&ch->lock);

        // Try to get the receiver right.
        if (ch->receiver != NULL) {
            return ERR_ALREADY_RECEVING;
        }

        // Resume a thread in the sender queue if exists.
        struct list_head *node = list_pop_front(&ch->sender_queue);
        if (node) {
            struct thread *thread = LIST_CONTAINER(thread, sender_queue_elem, node);
            thread_resume(thread);
        }

        // The sender thread will resume us.
        ch->receiver = CURRENT;
        thread_block(CURRENT);
        spin_unlock_irqrestore(&ch->lock, flags);
        thread_switch();

        // Now `CURRENT->buf` is filled by the sender thread.
        __asm__ __volatile__(
            // "movq %%r, %%rdi   \n"
            "movq 0(%%rax), %%rsi   \n"
            "movq 8(%%rax), %%rdx   \n"
            "movq 16(%%rax), %%r10   \n"
            "movq 24(%%rax), %%r8    \n"
            "movq 32(%%rax), %%r9    \n"
        :: "D"(CURRENT->header), "a"(CURRENT->buf)
        );
        return OK;
    }

    return OK;
}
