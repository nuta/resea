#include <channel.h>
#include <process.h>
#include <memory.h>
#include <thread.h>

/// Creates a channel in the given process. Returns NULL if failed.
struct channel *channel_create(struct process *process) {
    int cid = table_alloc(&process->channels);
    if (!cid) {
        TRACE("failed to allocate cid");
        return NULL;
    }

    struct channel *channel = KMALLOC(&object_arena, sizeof(struct channel));
    if (!channel) {
        TRACE("failed to allocate a channel");
        table_free(&process->channels, cid);
        return NULL;
    }

    channel->cid = cid;
    channel->ref_count = 1;
    channel->destructed = false;
    channel->process = process;
    channel->linked_with = channel;
    channel->transfer_to = channel;
    channel->receiver = NULL;
    channel->notification = 0;
    list_init(&channel->queue);

    table_set(&process->channels, cid, channel);
    list_push_back(&process->channel_list, &channel->next);
    return channel;
}

/// Increments the reference counter of the channel.
void channel_incref(struct channel *ch) {
    ASSERT(ch->ref_count >= 1);
    ch->ref_count++;
}

/// Increments the reference counter of the channel.
/// TODO: Not tested!
void channel_decref(struct channel *ch) {
    ASSERT(ch->ref_count >= 1);
    ch->ref_count--;

    if (ch->ref_count > 0) {
        return;
    }

    //
    //  Now the channel is no longer used. Free its resources.
    //

    // Unlink the channel.
    channel_link(ch, ch);
    channel_transfer(ch, ch);

    // Resume the receiver thread to abort its IPC operations.
    struct thread *receiver = ch->receiver;
    if (receiver) {
        receiver->abort_reason = ERR_CHANNEL_CLOSED;
        thread_resume(receiver);
    }

    // Resume the sender threads to abort theier IPC operations.
    LIST_FOR_EACH(sender, &ch->queue, struct thread, next) {
        sender->abort_reason = ERR_CHANNEL_CLOSED;
        thread_resume(sender);
    }

    table_free(&ch->process->channels, ch->cid);
    list_remove(&ch->next);
#ifdef DEBUG_BUILD
    ch->process     = INVALID_POINTER;
    ch->linked_with = INVALID_POINTER;
    ch->transfer_to = INVALID_POINTER;
    ch->receiver    = INVALID_POINTER;
#endif
    kfree(&object_arena, ch);
}

/// Decrements the refrence counter of the channel and destructs a channel if
/// possible.
/// TODO: Not tested!
void channel_destroy(struct channel *ch) {
    DEBUG_ASSERT(!ch->destructed);
    ch->destructed = true;

    channel_decref(ch);
}

/// Links two channels. The message from `ch1` will be sent to `ch2`. `ch1` and
/// `ch2` can be the same channel.
void channel_link(struct channel *ch1, struct channel *ch2) {
    if (ch1 == ch2) {
        if (ch1->linked_with != ch1) {
            channel_decref(ch1->linked_with);
        }

        if (ch2->linked_with != ch2) {
            channel_decref(ch2->linked_with);
        }

        ch1->linked_with = ch1;
    } else {
        // TODO: decref old linked channels.
        ch1->linked_with = ch2;
        ch2->linked_with = ch1;
        channel_incref(ch1);
        channel_incref(ch2);
    }
}

/// Transfers messages to `src` to `dst`. `src` and `dst` must be in the same
/// process.
void channel_transfer(struct channel *src, struct channel *dst) {
    ASSERT(src->process == dst->process);

    if (src == dst) {
        if (src->transfer_to != src) {
            channel_decref(src->transfer_to);
        }
        src->transfer_to = src;
    } else {
        src->transfer_to = dst;
        channel_incref(dst);
    }
}

/// Sends a notification.
error_t channel_notify(struct channel *ch, notification_t notification) {
    // Update the notification data.
    struct channel *dst = ch->linked_with->transfer_to;
    dst->notification |= notification;

    TRACE("notify: %pC -> %pC => %pC (data=%p)",
          ch, ch->linked_with, dst, dst->notification);

    // Resume the receiver thread if exists.
    struct thread *receiver = dst->receiver;
    if (receiver) {
        receiver->ipc_buffer->header = NOTIFICATION_NOTIFICATION_MSG;
        receiver->ipc_buffer->from   = 0;
        thread_resume(receiver);
        dst->receiver = NULL;
    }

    return OK;
}
