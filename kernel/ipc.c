#include <ipc.h>
#include <list.h>
#include <printk.h>
#include <process.h>
#include <server.h>
#include <table.h>
#include <thread.h>

/// Creates a channel in the given process. Returns NULL if failed.
struct channel *channel_create(struct process *process) {
    int cid = idtable_alloc(&process->channels);
    if (!cid) {
        TRACE("failed to allocate cid");
        return NULL;
    }

    struct channel *channel = kmalloc(&page_arena);
    if (!channel) {
        TRACE("failed to allocate a channel");
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
    list_init(&channel->queue);

    idtable_set(&process->channels, cid, channel);
    return channel;
}

/// Increments the reference counter of the channel.
void channel_incref(struct channel *ch) {
    ch->ref_count++;
}

/// Decrements the reference counter of the channel.
void channel_decref(struct channel *ch) {
    ch->ref_count--;
    ASSERT(ch->ref_count >= 0);

    if (ch->ref_count == 0) {
        // The channel is no longer referenced. Free resources.
        UNIMPLEMENTED();
    }
}

/// Links two channels. The message from `ch1` will be sent to `ch2`. `ch1` and
/// `ch2` can be the same channel.
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

/// Transfers messages to `src` to `dst`. `src` and `dst` must be in the same
/// process.
void channel_transfer(struct channel *src, struct channel *dst) {
    ASSERT(src->process == dst->process);

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

/// The open system call: creates a new channel.
cid_t sys_open(void) {
    struct channel *ch = channel_create(CURRENT->process);
    if (!ch) {
        return ERR_OUT_OF_RESOURCE;
    }

    TRACE("open: @%d", ch->cid);
    return ch->cid;
}

/// The close system call: destroys a new channel.
error_t sys_close(cid_t cid) {
    struct channel *ch = idtable_get(&CURRENT->process->channels, cid);
    if (!ch) {
        return ERR_INVALID_CID;
    }

    TRACE("close: @%d", ch->cid);
    channel_decref(ch);
    return OK;
}

/// The link system call: links channels.
error_t sys_link(cid_t ch1, cid_t ch2) {
    struct channel *ch1_ch = idtable_get(&CURRENT->process->channels, ch1);
    if (!ch1_ch) {
        return ERR_INVALID_CID;
    }

    struct channel *ch2_ch = idtable_get(&CURRENT->process->channels, ch2);
    if (!ch2_ch) {
        return ERR_INVALID_CID;
    }

    TRACE("link: @%d->@%d", ch1_ch->cid, ch2_ch->cid);
    channel_link(ch1_ch, ch2_ch);
    return OK;
}

/// The transfer system call: transfers messages.
error_t sys_transfer(cid_t src, cid_t dst) {
    struct channel *src_ch = idtable_get(&CURRENT->process->channels, src);
    if (!src_ch) {
        return ERR_INVALID_CID;
    }

    struct channel *dst_ch = idtable_get(&CURRENT->process->channels, dst);
    if (!dst_ch) {
        return ERR_INVALID_CID;
    }

    TRACE("trasnfer: @%d->@%d", src_ch->cid, dst_ch->cid);
    channel_transfer(src_ch, dst_ch);
    return OK;
}

/// Resolves the physical address linked to the given virtual address.
static paddr_t resolve_paddr(struct page_table *page_table, vaddr_t vaddr) {
    paddr_t paddr = resolve_paddr_from_vaddr(page_table, vaddr);
    if (paddr) {
        return paddr;
    }

    // The page does not exist in the page table. Invoke the page fault handler
    // since perhaps it is just not filled by the pager (i.e., not yet accessed
    // by the user). Note that page_fault_handler always returns a valid paddr;
    // if the vaddr is invalid, it kills the current thread and won't return.
    TRACE("page payload %p does not exist, trying the pager...", vaddr);
    return page_fault_handler(vaddr, PF_USER);
}

/// The ipc system call: sends/receives messages.
///
/// TODO: Can we replace spin_lock_irqsave with spin_lock?
///
error_t sys_ipc(cid_t cid, uint32_t syscall) {
    struct thread *current = CURRENT;
    struct channel *ch = idtable_get(&current->process->channels, cid);
    if (!ch) {
        return ERR_INVALID_CID;
    }

    bool from_kernel = (syscall & IPC_FROM_KERNEL) != 0;

    //
    //  Send Phase
    //
    if (syscall & IPC_SEND) {
        struct message *m = from_kernel ? current->kernel_ipc_buffer
                                        : &current->info->ipc_buffer;
        header_t header = m->header;
        flags_t flags = spin_lock_irqsave(&ch->lock);
        struct channel *linked_to = ch->linked_to;
        struct channel *dst = linked_to->transfer_to;
        struct thread *receiver;

        if (INTERFACE_ID(header) != 4) {
            TRACE("send: %*c -> %*c (header=%p)", ch, dst, header);
        }

        spin_unlock_irqrestore(&ch->lock, flags);

        // Wait for a receiver thread.
        while (1) {
            flags_t flags = spin_lock_irqsave(&dst->lock);
            receiver = dst->receiver;
            if (receiver != NULL) {
                dst->receiver = NULL;
                spin_unlock_irqrestore(&dst->lock, flags);
                break;
            }

            // A receiver is not ready or another thread is sending a message
            // to the channel. Block the current thread until a receiver thread
            // resumes us.
            list_push_back(&dst->queue, &current->queue_elem);
            thread_block(current);
            spin_unlock_irqrestore(&dst->lock, flags);
            thread_switch();
        }

        // Now we have a receiver thread. It's time to send a message!
        struct message *dst_m = receiver->ipc_buffer;

        // Set header and the source channel ID.
        dst_m->header = header;
        dst_m->from = linked_to->cid;

        // Copy inline payloads.
        memcpy(dst_m->data, INLINE_PAYLOAD_LEN_MAX, m->data,
            INLINE_PAYLOAD_LEN(header));

        // Copy page payloads.
        struct page_table *page_table = &current->process->page_table;
        for (size_t i = 0; i < PAGE_PAYLOAD_NUM(header); i++) {
            page_t page = m->pages[i];
            int page_type = PAGE_TYPE(page);

            // TODO: Support multiple pages.
            ASSERT(PAGE_EXP(page) == 0);
            // TODO: Support PAGE_TYPE_MOVE.
            ASSERT(page_type == PAGE_TYPE_SHARED);

            if (receiver->recv_in_kernel) {
                dst_m->pages[i] = resolve_paddr(page_table, page);
            } else {
                UNIMPLEMENTED();
            }
        }

        // TODO: Handle channel payloads.
        ASSERT(CHANNELS_PAYLOAD_NUM(header) == 0);

        thread_resume(receiver);
    }

    //
    //  Receive Phase
    //
    if (syscall & IPC_RECV) {
        flags_t flags = spin_lock_irqsave(&ch->lock);
        struct channel *recv_on = ch->transfer_to;
        spin_unlock_irqrestore(&ch->lock, flags);
        flags = spin_lock_irqsave(&recv_on->lock);

        // Try to get the receiver right.
        if (recv_on->receiver != NULL) {
            return ERR_ALREADY_RECEVING;
        }

        current->recv_in_kernel = from_kernel;
        if (from_kernel) {
            current->ipc_buffer = current->kernel_ipc_buffer;
        }

        recv_on->receiver = current;
        thread_block(current);

        // Resume a thread in the sender queue if exists.
        struct list_head *node = list_pop_front(&recv_on->queue);
        if (node) {
            thread_resume(LIST_CONTAINER(thread, queue_elem, node));
        }

        // Wait for a message...
        spin_unlock_irqrestore(&recv_on->lock, flags);
        thread_switch();

        // Now `CURRENT->ipc_buffer` is filled by the sender thread.

        if (INTERFACE_ID(current->ipc_buffer->header) != 4) {
            TRACE("recv: %*c <- @%d (header=%p)", recv_on,
                current->ipc_buffer->from, current->ipc_buffer->header);
        }

        if (from_kernel) {
            current->ipc_buffer = &current->info->ipc_buffer;
        }
    }

    return OK;
}

/// The ipc system call (faster version): it optmizes a common case: payloads
/// are inline only (i.e., no channel/page payloads), both IPC_SEND and
/// IPC_RECV are specified, and a receiver thread already waits for a message.
///
/// If preconditions are not met, it fall backs into the full-featured version
/// (`sys_ipc()`).
///
/// Note that the current implementation is not fast enough. We need to
/// eliminate branches and memory accesses as much as possible.
error_t sys_ipc_fastpath(cid_t cid) {
    struct thread *current = CURRENT;
    struct channel *ch = idtable_get(&current->process->channels, cid);
    if (UNLIKELY(!ch)) {
        goto slowpath1;
    }

    struct message *m = current->ipc_buffer;
    header_t header = m->header;

    // Fastpath accepts only inline payloads.
    if (UNLIKELY(SYSCALL_FASTPATH_TEST(header) != 0)) {
        goto slowpath1;
    }

    // Try locking the channel. If it failed, enter the slowpath intead of
    // waiting for it here because spin_lock() would be large for inlining.
    if (UNLIKELY(!spin_try_lock(&ch->lock))) {
        goto slowpath1;
    }

    // Lock the channel where we will wait for a message if necessary.
    struct channel *recv_on = ch->transfer_to;
    if (UNLIKELY(ch != recv_on && !spin_try_lock(&recv_on->lock))) {
        goto slowpath2;
    }

    // Make sure that no threads are waitng for us.
    if (UNLIKELY(!list_is_empty(&recv_on->queue))) {
        goto slowpath3;
    }

    struct channel *linked_to = ch->linked_to;
    struct channel *dst_ch = linked_to->transfer_to;

    // Try locking the destination channel.
    if (UNLIKELY(!spin_try_lock(&dst_ch->lock))) {
        goto slowpath3;
    }

    // Make sure that a receiver thread is already waiting on the destination
    // channel.
    struct thread *receiver = dst_ch->receiver;
    if (UNLIKELY(!receiver)) {
        goto slowpath4;
    }

    // Now all prerequisites are met. Copy the message and wait on the our
    // channel.
    if (INTERFACE_ID(header) != 4) {
        TRACE("send (fastpath): %*c -> %*c (header=%p)", ch, dst_ch, header);
    }

    struct message *dst_m = receiver->ipc_buffer;
    dst_m->header = header;
    dst_m->from = linked_to->cid;
    memcpy_unchecked(&dst_m->data, m->data, INLINE_PAYLOAD_LEN(header));

    recv_on->receiver = current;
    current->state = THREAD_BLOCKED;
    receiver->state = THREAD_RUNNABLE;

    dst_ch->receiver = NULL;
    spin_unlock(&dst_ch->lock);
    spin_unlock(&ch->lock);
    if (ch != ch->transfer_to) {
        spin_unlock(&recv_on->lock);
    }

    // Do a direct context switch into the receiver thread. The current thread
    // is now blocked and will be resumed by another IPC call.
    arch_thread_switch(current, receiver);

    // Resumed by a sender thread.
    return OK;

slowpath4:
    spin_unlock(&dst_ch->lock);
slowpath3:
    if (ch != ch->transfer_to) {
        spin_unlock(&recv_on->lock);
    }
slowpath2:
    spin_unlock(&ch->lock);
slowpath1:
    return sys_ipc(cid, IPC_SEND | IPC_RECV);
}

/// The system call handler to be called from the arch's handler.
intmax_t syscall_handler(uintmax_t arg0, uintmax_t arg1, UNUSED uintmax_t arg3,
    uintmax_t syscall) {

    // Try IPC fastpath if possible.
    if (LIKELY(syscall == (SYSCALL_IPC | IPC_SEND | IPC_RECV))) {
        // TODO: Support SYSCALL_REPLY.
        return sys_ipc_fastpath(arg0);
    }

    // IPC_FROM_KERNEL must not be specified by the user.
    if (syscall & IPC_FROM_KERNEL) {
        return ERR_INVALID_HEADER;
    }

    switch (SYSCALL_TYPE(syscall)) {
    case SYSCALL_IPC:
        return sys_ipc(arg0, (uint32_t) syscall);
    case SYSCALL_OPEN:
        return sys_open();
    case SYSCALL_CLOSE:
        return sys_close(arg0);
    case SYSCALL_LINK:
        return sys_link(arg0, arg1);
    case SYSCALL_TRANSFER:
        return sys_transfer(arg0, arg1);
    default:
        return ERR_INVALID_SYSCALL;
    }
}
