#include <config.h>
#include <channel.h>
#include <process.h>
#include <channel.h>
#include <thread.h>
#include <ipc.h>
#include <idl_messages.h>

/// The open system call: creates a new channel. It returns negated error value
/// if an error occurred.
int sys_open(void) {
    STATIC_ASSERT(sizeof(int) == sizeof(cid_t));

    struct channel *ch = channel_create(CURRENT->process);
    if (!ch) {
        return -ERR_OUT_OF_RESOURCE;
    }

    TRACE("sys_open: created %pC", ch);
    return ch->cid;
}

/// The close system call: destroys a new channel.
error_t sys_close(cid_t cid) {
    struct channel *ch = table_get(&CURRENT->process->channels, cid);
    if (!ch) {
        return ERR_INVALID_CID;
    }

    TRACE("sys_close: destructing %pC", ch);
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

    TRACE("sys_link: linking %pC <-> %pC", ch1_ch, ch2_ch);
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

    TRACE("sys_trasnfer: transfering %pC -> %pC", src_ch, dst_ch);
    channel_transfer(src_ch, dst_ch);
    return OK;
}

/// Performs an IPC from kernel threads.
error_t kernel_ipc(cid_t cid, uint32_t syscall) {
    // Switch to the thread-local kernel IPC buffer. Kernel threads can't use
    // the buffer in the TIB because page faults could be occurred during IPC
    // preparation in userland. Consider the following example:
    //
    // ```
    //    ipc_buffer->header = PRINTCHAR_HEADER;
    //    ipc_buffer->data[0] = foo(); // Page fault occurs here!
    //    // If kernel and user share one ipc_buffer, the buffer is
    //    // overwritten by the page fault handler. Consequently,
    //    // ipc_buffer->header is no longer equal to PRINTCHAR_HEADER!
    //    //
    //    // This is why we need a kernel's own ipc buffer.
    //    sys_ipc(ch);
    // ```
    //
    struct thread *current = CURRENT;
    current->ipc_buffer = current->kernel_ipc_buffer;

    error_t err = sys_ipc(cid, syscall);

    current->ipc_buffer = &current->info->ipc_buffer;
    return err;
}

/// The ipc system call: sends/receives messages.
error_t sys_ipc(cid_t cid, uint32_t syscall) {
    DEBUG_ASSERT(
        (CURRENT->process != kernel_process
         || CURRENT->ipc_buffer == CURRENT->kernel_ipc_buffer)
        && "Don't use sys_ipc() directly from a kernel thread; use kernel_ipc()"
    );

    struct thread *current = CURRENT;
    struct message *m = current->ipc_buffer;
    struct channel *ch = table_get(&current->process->channels, cid);
    if (!ch) {
        return ERR_INVALID_CID;
    }

    if (ch->destructed) {
        return ERR_CHANNEL_CLOSED;
    }

    //
    //  Send Phase
    //
    if (syscall & IPC_SEND) {
        header_t header = m->header;
        struct channel *linked_with;
        struct channel *dst;
        struct thread *receiver;
#ifdef DEBUG_BUILD
        current->debug.send_from = ch;
#endif
        // Wait for a receiver thread.
        while (1) {
            linked_with = ch->linked_with;
            dst = linked_with->transfer_to;
            receiver = dst->receiver;

            if (dst->destructed) {
                return ERR_CHANNEL_CLOSED;
            }

            // Check if there's a thread waiting for a message on the
            // destination channel and there're no pending notification in the
            // channel.
            if (receiver != NULL) {
                dst->receiver = NULL;
                break;
            }

            // Exit the system call handler if IPC_NOBLOCK is specified.
            if (syscall & IPC_NOBLOCK) {
                return ERR_WOULD_BLOCK;
            }

            // A receiver is not ready or another thread is sending a message
            // to the channel. Block the current thread until a receiver thread
            // resumes us.
            list_push_back(&dst->queue, &current->queue_next);
            current->blocked_on = dst;
            thread_block(current);
            thread_switch();

            if (current->abort_reason != OK) {
                return atomic_swap(&current->abort_reason, OK);
            }
        }

#ifdef DEBUG_BUILD
        current->debug.send_from = NULL;
#endif
        IPC_TRACE(m, "send: %pC -> %pC => %pC (header=%p)",
                  ch, linked_with, dst, header);

        // Now we have a receiver thread. It's time to send a message!
        struct message *dst_m = receiver->ipc_buffer;

        // Set header and the source channel ID.
        dst_m->header = header;
        dst_m->from = linked_with->cid;

        // Copy inline payloads.
        inlined_memcpy(dst_m->payloads.data, m->payloads.data,
                       INLINE_PAYLOAD_LEN(header));

        // Handle the channel payload.
        if (header & MSG_CHANNEL_PAYLOAD) {
            struct channel *payload_ch = table_get(&current->process->channels,
                                           m->payloads.channel);
            if (!payload_ch) {
                receiver->abort_reason = ERR_NEEDS_RETRY;
                return ERR_INVALID_PAYLOAD;
            }

            struct channel *dst_ch = channel_create(receiver->process);
            if (!dst_ch) {
                receiver->abort_reason = ERR_NEEDS_RETRY;
                return ERR_OUT_OF_MEMORY;
            }

            channel_link(payload_ch->linked_with, dst_ch);
            dst_m->payloads.channel = dst_ch->cid;
        }

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

            if (receiver->ipc_buffer == receiver->kernel_ipc_buffer) {
                // Kernel threads prefers the physical address (e.g, receiving a
                // pager.fill_request reply).
                dst_m->payloads.page = paddr;
            } else {
                // Make sure that the receiver accepts a page payload and its
                // base address is valid.
                if (PAGE_ORDER(page_base) < PAGE_ORDER(page)
                    || !is_valid_page_base_addr(page_base_addr)) {
                    receiver->abort_reason = ERR_NEEDS_RETRY;
                    return ERR_INVALID_PAGE_PAYLOAD;
                }

                link_page(&receiver->process->page_table, page_base_addr, paddr,
                          num_pages, PAGE_USER | PAGE_WRITABLE);
                dst_m->payloads.page = PAGE_PAYLOAD(page_base_addr, PAGE_ORDER(page));
            }

            // Unlink the pages from the current (sender) process.
            unlink_page(&current->process->page_table, PAGE_PAYLOAD_ADDR(page),
                        num_pages);
        }

        thread_resume(receiver);
    }

    //
    //  Receive Phase
    //
    if (syscall & IPC_RECV) {
        struct channel *recv_ch = ch->transfer_to;
        if (recv_ch->destructed) {
            return ERR_CHANNEL_CLOSED;
        }

        // Try to get the receiver right.
        if (recv_ch->receiver != NULL) {
            return ERR_ALREADY_RECEVING;
        }

#ifdef DEBUG_BUILD
        current->debug.receive_from = recv_ch;
#endif
        recv_ch->receiver = current;

        thread_block(current);

        // Resume a thread in the sender queue if exists.
        struct list_head *node = list_pop_front(&recv_ch->queue);
        if (node) {
            struct thread *sender = LIST_CONTAINER(node, struct thread,
                                                   queue_next);
            sender->blocked_on = NULL;
            thread_resume(sender);
        }

        // Wait for a message...
        thread_switch();

        // Received a message or a notification, or the IPC operation is
        // aborted.

        if (current->abort_reason != OK) {
            return atomic_swap(&current->abort_reason, OK);
        }

        // Read and clear the notification field atomically.
        m->notification = atomic_swap(&recv_ch->notification, 0);

        IPC_TRACE(m, "recv: %pC <- @%d (header=%p, notification=%p)",
                  recv_ch, m->from, m->header, m->notification);

#ifdef DEBUG_BUILD
        current->debug.receive_from = NULL;
#endif
    }

    return OK;
}

#ifdef CONFIG_FASTPATH
/// The faster ipc system call implementation optimized for the common case:
///
///   - Payloads are inline only (i.e., no channel/page payloads).
///   - Both IPC_SEND and  IPC_RECV are specified.
///   - A receiver thread already waits for a message.
///
/// If these preconditions are not met, it fall backs into the full-featured
/// version (`sys_ipc()`).
///
/// Note that the current implementation is not fast enough. We need to
/// eliminate memory accesses...
static error_t sys_ipc_fastpath(cid_t cid) {
    DEBUG_ASSERT(CURRENT->process != kernel_process);

    struct thread *current = CURRENT;
    struct channel *ch = table_get(&current->process->channels, cid);
    if (UNLIKELY(!ch)) {
        goto slowpath_fallback;
    }

    struct message *m           = current->ipc_buffer;
    header_t header             = m->header;
    struct channel *recv_ch     = ch->transfer_to;
    struct channel *linked_with = ch->linked_with;
    cid_t from                  = linked_with->cid;
    struct channel *dst_ch      = linked_with->transfer_to;
    struct thread *receiver     = dst_ch->receiver;

    // Check whether the message can be sent in fastpath. Here we use `+`
    // instead of lengthy if statements to eliminate branches.
    int slowpath =
        // Fastpath accepts only inline payloads.
        !FASTPATH_HEADER_TEST(header) +
        // Make sure that the channels are not destructed.
        ch->destructed + recv_ch->destructed + dst_ch->destructed +
        // Make sure that the current thread is able to be the receiver of
        // `recv_ch`.
        ((int) recv_ch->receiver) +
        // Make sure that no threads are waitng for us.
        !list_is_empty(&recv_ch->queue) +
        // A receiver thread already waits for a message.
        !receiver;

    // Fall back into the slowpath if the conditions are not met.
    if (slowpath) {
        goto slowpath_fallback;
    }

    // Now all prerequisites are met. Copy the message and wait on the our
    // channel. We don't need to recv_in_kernel; it is already set to false in
    // sys_ipc().
    IPC_TRACE(m, "send (fastpath): %pC -> %pC => %pC (header=%p)",
              ch, linked_with, dst_ch, header);
    struct message *dst_m = receiver->ipc_buffer;
    recv_ch->receiver = current;
    dst_ch->receiver  = NULL;
    current->blocked  = true;
    receiver->blocked = false;
    dst_m->header     = header;
    dst_m->from       = from;
    inlined_memcpy(&dst_m->payloads.data, m->payloads.data,
                   INLINE_PAYLOAD_LEN(header));
#ifdef DEBUG_BUILD
    current->debug.receive_from = recv_ch;
#endif

    // Do a direct context switch into the receiver thread. The current thread
    // is now blocked and will be resumed by another IPC or when the send or
    // receive operation is aborted.
    arch_thread_switch(current, receiver);

    // Received a message or a notification, or the IPC operation is aborted.

    // Read and clear the notification field.
    m->notification = atomic_swap(&recv_ch->notification, 0);
    IPC_TRACE(m, "recv (fastpath): %pC <- @%d (header=%p, notification=%p)",
              recv_ch, m->from, m->header, m->notification);
#ifdef DEBUG_BUILD
    current->debug.receive_from = NULL;
#endif
    return atomic_swap(&current->abort_reason, OK);

slowpath_fallback:
    return sys_ipc(cid, IPC_SEND | IPC_RECV);
}
#endif // CONFIG_FASTPATH

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
int syscall_handler(uintmax_t arg0, uintmax_t arg1, uintmax_t syscall) {
    DEBUG_ASSERT(CURRENT->process != kernel_process);

#ifdef CONFIG_FASTPATH
    // Try IPC fastpath if possible.
    if (LIKELY(FASTPATH_SYSCALL_TEST(syscall))) {
        return sys_ipc_fastpath(arg0);
    }
#endif

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
