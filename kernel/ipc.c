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
    UNIMPLEMENTED();
}

void channel_link(struct channel *ch1, struct channel *ch2) {
    if (ch1 == ch2) {
        flags_t flags = spin_lock_irqsave(&ch1->lock);
        if (ch1->linked_to != ch1) {
            ch1->linked_to->ref_count--;
        }

        ch1->transfer_to = ch1;
        spin_unlock_irqrestore(&ch1->lock, flags);
    } else {
        flags_t flags = spin_lock_irqsave(&ch1->lock);
        spin_lock(&ch2->lock);

        ch1->linked_to = ch2;
        ch2->linked_to = ch1;
        ch1->ref_count++;
        ch2->ref_count++;

        spin_unlock(&ch2->lock);
        spin_unlock_irqrestore(&ch1->lock, flags);
    }
}

void channel_transfer(struct channel *src, struct channel *dst) {
    if (src == dst) {
        flags_t flags = spin_lock_irqsave(&src->lock);
        if (src->transfer_to != src) {
            src->transfer_to->ref_count--;
        }

        src->transfer_to = src;
        spin_unlock_irqrestore(&src->lock, flags);
    } else {
        flags_t flags = spin_lock_irqsave(&src->lock);
        spin_lock(&dst->lock);

        src->transfer_to = dst;
        dst->ref_count++;

        spin_unlock(&dst->lock);
        spin_unlock_irqrestore(&src->lock, flags);
    }
}

cid_t sys_open(void) {
    struct channel *ch = channel_create(CURRENT->process);
    if (!ch) {
        return ERR_OUT_OF_RESOURCE;
    }

    DEBUG("open: @%d", ch->cid);
    return ch->cid;
}

error_t sys_link(cid_t ch1, cid_t ch2) {
    struct channel *ch1_ch = idtable_get(&CURRENT->process->channels, ch1);
    if (!ch1_ch) {
        return ERR_INVALID_CID;
    }

    struct channel *ch2_ch = idtable_get(&CURRENT->process->channels, ch2);
    if (!ch2_ch) {
        return ERR_INVALID_CID;
    }

    DEBUG("link: @%d->@%d", ch1_ch->cid, ch2_ch->cid);
    channel_link(ch1_ch, ch2_ch);
    return OK;
}

error_t sys_transfer(cid_t src, cid_t dst) {
    struct channel *src_ch = idtable_get(&CURRENT->process->channels, src);
    if (!src_ch) {
        return ERR_INVALID_CID;
    }

    struct channel *dst_ch = idtable_get(&CURRENT->process->channels, dst);
    if (!dst_ch) {
        return ERR_INVALID_CID;
    }

    DEBUG("trasnfer: @%d->@%d", src_ch->cid, dst_ch->cid);
    channel_transfer(src_ch, dst_ch);
    return OK;
}

error_t sys_ipc(cid_t cid, int options) {
    struct channel *ch = idtable_get(&CURRENT->process->channels, cid);
    if (!ch) {
        return ERR_INVALID_CID;
    }

    payload_t header = CURRENT->buffer->header;
    if (ch->linked_to->process == kernel_process) {
        if (!FLAG_SEND(options) || !FLAG_RECV(options)) {
            DEBUG("kernel server: invalid options");
            return ERR_INVALID_HEADER;
        }

        kernel_server();
        return OK;
    }

    if (FLAG_SEND(options)) {
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
        struct message *src_buffer = CURRENT->buffer;
        struct message *dst_buffer = receiver->buffer;

        // Copy inline payloads.
        memcpy(dst_buffer->inlines, INLINE_PAYLOAD_LEN_MAX,
            src_buffer->inlines, INLINE_PAYLOAD_LEN(header));

        // Copy page payloads.
        for (size_t i = 0; i < PAGE_PAYLOAD_NUM(header); i++) {
            /* FIXME:
            if (thread->process != kernel_process && INTERFACE_ID(header) != PAGER_INTERFACE) {
                UNIMPLEMENTED();
            }
            */

            vaddr_t vaddr = ALIGN_DOWN(src_buffer->pages[i], PAGE_SIZE);
            paddr_t paddr = arch_resolve_paddr_from_vaddr(
                &CURRENT->process->page_table, vaddr);

            if (!paddr) {
                // TODO: return error
                UNIMPLEMENTED();
            }
            dst_buffer->pages[i] = paddr;
        }

        // Set header and the source channel ID.
        dst_buffer->header = header;
        dst_buffer->from = linked_to->cid;

        thread_resume(receiver);
        dst->receiver = NULL;
        spin_unlock_irqrestore(&dst->lock, flags);
    }

    if (FLAG_RECV(options)) {
        struct channel *recv_on;
        if (FLAG_REPLY(options)) {
            recv_on = ch->transfer_to;
        } else {
            recv_on = ch;
        }

        flags_t flags = spin_lock_irqsave(&recv_on->lock);

        // Try to get the receiver right.
        if (recv_on->receiver != NULL) {
            return ERR_ALREADY_RECEVING;
        }

        // Resume a thread in the sender queue if exists.
        struct list_head *node = list_pop_front(&recv_on->sender_queue);
        if (node) {
            struct thread *thread = LIST_CONTAINER(thread, sender_queue_elem, node);
            thread_resume(thread);
        }

        // The sender thread will resume us.
        recv_on->receiver = CURRENT;
        thread_block(CURRENT);
        spin_unlock_irqrestore(&recv_on->lock, flags);
        thread_switch();

        // Now `CURRENT->buffer` is filled by the sender thread.
        return OK;
    }

    return OK;
}
