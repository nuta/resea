#include <channel.h>
#include <process.h>
#include <channel.h>
#include <thread.h>
#include <syscall.h>
#include <resea_idl.h>

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
    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    if (!ch) {
        return ERR_INVALID_CID;
    }

    TRACE("close: @%d", ch->cid);
    channel_destroy(ch);
    return OK;
}

/// The link system call: links channels.
error_t sys_link(cid_t ch1, cid_t ch2) {
    struct channel *ch1_ch = table_get(&CURRENT->process->channels, ch1);
    if (!ch1_ch) {
        return ERR_INVALID_CID;
    }

    struct channel *ch2_ch = table_get(&CURRENT->process->channels, ch2);
    if (!ch2_ch) {
        return ERR_INVALID_CID;
    }

    TRACE("link: @%d->@%d", ch1_ch->cid, ch2_ch->cid);
    channel_link(ch1_ch, ch2_ch);
    return OK;
}

/// The transfer system call: transfers messages.
error_t sys_transfer(cid_t src, cid_t dst) {
    struct channel *src_ch = table_get(&CURRENT->process->channels, src);
    if (!src_ch) {
        return ERR_INVALID_CID;
    }

    struct channel *dst_ch = table_get(&CURRENT->process->channels, dst);
    if (!dst_ch) {
        return ERR_INVALID_CID;
    }

    TRACE("trasnfer: @%d->@%d", src_ch->cid, dst_ch->cid);
    channel_transfer(src_ch, dst_ch);
    return OK;
}

/// The ipc system call: sends/receives messages.
error_t sys_ipc(cid_t cid, uint32_t syscall) {
    struct thread *current = CURRENT;
    struct channel *ch = table_get(&current->process->channels, cid);
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
        struct channel *linked_to;
        struct channel *dst;
        struct thread *receiver;
#ifdef DEBUG_BUILD
        current->debug.send_from = ch;
#endif
        // Wait for a receiver thread.
        while (1) {
            flags_t flags = spin_lock_irqsave(&ch->lock);
            linked_to = ch->linked_to;
            dst = linked_to->transfer_to;
            spin_lock(&dst->lock);
            receiver = dst->receiver;

            // Check if there's a thread waiting for a message on the
            // destination channel and there're no pending notification in the
            // channel.
            if (receiver != NULL) {
                dst->receiver = NULL;
                spin_unlock(&dst->lock);
                spin_unlock_irqrestore(&ch->lock, flags);
                break;
            }

            // Exit the system call handler if IPC_NOBLOCK is specified.
            if (syscall & IPC_NOBLOCK) {
                spin_unlock(&dst->lock);
                spin_unlock_irqrestore(&ch->lock, flags);
                return ERR_WOULD_BLOCK;
            }

            // A receiver is not ready or another thread is sending a message
            // to the channel. Block the current thread until a receiver thread
            // resumes us.
            list_push_back(&dst->queue, &current->queue_elem);
            thread_block(current);
            spin_unlock(&dst->lock);
            spin_unlock_irqrestore(&ch->lock, flags);
            thread_switch();
        }

#ifdef DEBUG_BUILD
        current->debug.send_from = NULL;
#endif
        IPC_TRACE(m, "send: %pC -> %pC => %pC (header=%p)",
                  ch, linked_to, dst, header);

        // Now we have a receiver thread. It's time to send a message!
        struct message *dst_m = receiver->ipc_buffer;

        // Set header and the source channel ID.
        dst_m->header = header;
        dst_m->from = linked_to->cid;

        // Copy inline payloads.
        inlined_memcpy(dst_m->payloads.data, m->payloads.data,
                       INLINE_PAYLOAD_LEN(header));

        // Copy the page payload if exists.
        if (header & MSG_PAGE_PAYLOAD) {
            page_t page = m->payloads.page;
            vaddr_t payload_vaddr = PAGE_PAYLOAD_ADDR(page);
            page_t page_base = receiver->info->page_base;
            vaddr_t page_base_addr = PAGE_PAYLOAD_ADDR(page_base);
            int num_pages = POW2(PAGE_ORDER(page));

            // Resolve the physical address referenced by the page payload.
            struct page_table *page_table = &current->process->page_table;
            paddr_t paddr = resolve_paddr_from_vaddr(page_table, payload_vaddr);
            if (!paddr) {
                // The page does not exist in the page table. Invoke the page
                // fault handler since perhaps it is just not filled by the
                // pager (i.e., not yet accessed by the user). Note that
                // page_fault_handler always returns a valid paddr;  if the
                // vaddr is invalid, it kills the current thread and won't
                // return.
                paddr = page_fault_handler(payload_vaddr, PF_USER);
            }

            if (receiver->recv_in_kernel) {
                // Kernel (receiving a pager.fill_request reply, for example)
                // links the page by itself only if necessary.
                dst_m->payloads.page = paddr;
            } else {
                // Make sure that the receiver accepts a page payload and its
                // base address is valid.
                if (PAGE_ORDER(page_base) < PAGE_ORDER(page)
                    || !is_valid_page_base_addr(page_base_addr)) {
                    return ERR_UNACCEPTABLE_PAGE_PAYLOAD;
                }

                link_page(&receiver->process->page_table, page_base_addr, paddr,
                          num_pages, PAGE_USER | PAGE_WRITABLE);
                dst_m->payloads.page = PAGE_PAYLOAD(page_base_addr, PAGE_ORDER(page));
            }

            // Unlink the pages from the current (sender) process.
            unlink_page(&current->process->page_table, PAGE_PAYLOAD_ADDR(page),
                        num_pages);
        }

        // Handle the channel payload.
        if (header & MSG_CHANNEL_PAYLOAD) {
            struct channel *ch = table_get(&current->process->channels,
                                           m->payloads.channel);
            struct channel *dst_ch = channel_create(receiver->process);
            // FIXME: Clean up and return an error instead.
            ASSERT(ch);
            ASSERT(dst_ch);

            channel_link(ch->linked_to, dst_ch);
            // FIXME: channel_destroy(ch);
            dst_m->payloads.channel = dst_ch->cid;
        }

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

#ifdef DEBUG_BUILD
        current->debug.receive_from = recv_on;
#endif
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

        // Received a message or a notification.

#ifdef DEBUG_BUILD
        current->debug.receive_from = NULL;
#endif
        // TODO: Atomic swap.
        struct message *m = current->ipc_buffer;
        m->notification = recv_on->notification;
        recv_on->notification = 0;

        // Now `CURRENT->ipc_buffer` is filled by the sender thread.
        IPC_TRACE(m, "recv: %pC <- @%d (header=%p, notification=%p)",
                  recv_on, m->from, m->header, m->notification);

        if (from_kernel) {
            current->ipc_buffer = &current->info->ipc_buffer;
        }
    }

    return OK;
}

/// The ipc system call (faster version): it optmizes the common case:
///
///   - Payloads are inline only (i.e., no channel/page payloads).
///   - Both IPC_SEND and  IPC_RECV are specified.
///   - A receiver thread already waits for a message.
///
/// If preconditions are not met, it fall backs into the full-featured version
/// (`sys_ipc()`).
///
/// Note that the current implementation is not fast enough. We need to
/// eliminate branches and memory accesses as much as possible.
error_t sys_ipc_fastpath(cid_t cid) {
    struct thread *current = CURRENT;
    struct channel *ch = table_get(&current->process->channels, cid);
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
    IPC_TRACE(m, "send (fastpath): %pC -> %pC => %pC (header=%p)",
               ch, linked_to, dst_ch, header);

    struct message *dst_m = receiver->ipc_buffer;
    dst_m->header = header;
    dst_m->from = linked_to->cid;
    inlined_memcpy(&dst_m->payloads.data, m->payloads.data,
                   INLINE_PAYLOAD_LEN(header));

    recv_on->receiver = current;
    current->state = THREAD_BLOCKED;
    current->recv_in_kernel = false;
    receiver->state = THREAD_RUNNABLE;
    dst_ch->receiver = NULL;
#ifdef DEBUG_BUILD
    current->debug.receive_from = recv_on;
#endif
    spin_unlock(&dst_ch->lock);
    spin_unlock(&ch->lock);
    if (ch != ch->transfer_to) {
        spin_unlock(&recv_on->lock);
    }

    // Do a direct context switch into the receiver thread. The current thread
    // is now blocked and will be resumed by another IPC.
    arch_thread_switch(current, receiver);

#ifdef DEBUG_BUILD
    current->debug.receive_from = NULL;
#endif

    // TODO: Atomic swap.
    m->notification = recv_on->notification;
    recv_on->notification = 0;

    IPC_TRACE(m, "recv: %pC <- @%d (header=%p, notification=%p)",
              recv_on, m->from, m->header, m->notification);

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

/// The notify system call: sends a notification. This system call MUST be
/// asynchronous: return an error instead of blocking the current thread!
error_t sys_notify(cid_t cid, notification_t notification) {
    struct thread *current = CURRENT;
    struct channel *ch = table_get(&current->process->channels, cid);
    if (UNLIKELY(!ch)) {
        return ERR_INVALID_CID;
    }

    return channel_notify(ch, notification);
}

/// The system call handler to be called from the arch's handler.
intmax_t syscall_handler(uintmax_t arg0, uintmax_t arg1, uintmax_t syscall) {
    // Try IPC fastpath if possible.
    if (LIKELY(syscall == (SYSCALL_IPC | IPC_SEND | IPC_RECV))) {
        return sys_ipc_fastpath(arg0);
    }

    // IPC_FROM_KERNEL must not be specified by the user.
    if (syscall & IPC_FROM_KERNEL) {
        return ERR_INVALID_HEADER;
    }

    switch (SYSCALL_TYPE(syscall)) {
    case SYSCALL_IPC:
        return sys_ipc(arg0, syscall);
    case SYSCALL_OPEN:
        return sys_open();
    case SYSCALL_CLOSE:
        return sys_close(arg0);
    case SYSCALL_LINK:
        return sys_link(arg0, arg1);
    case SYSCALL_TRANSFER:
        return sys_transfer(arg0, arg1);
    case SYSCALL_NOTIFY:
        return sys_notify(arg0, arg1);
    default:
        return ERR_INVALID_SYSCALL;
    }
}
