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

    struct channel *channel = KMALLOC(&small_arena, sizeof(struct channel));
    if (!channel) {
        TRACE("failed to allocate a channel");
        table_free(&process->channels, cid);
        return NULL;
    }

    channel->cid = cid;
    channel->ref_count = 1;
    channel->process = process;
    channel->linked_with = channel;
    channel->transfer_to = channel;
    channel->receiver = NULL;
    channel->notification = 0;
    list_init(&channel->queue);

    table_set(&process->channels, cid, channel);
    return channel;
}

/// Increments the reference counter of the channel.
void channel_incref(struct channel *ch) {
    ch->ref_count++;
}

/// Decrements the reference counter of the channel.
void channel_destroy(struct channel *ch) {
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
        if (ch1->linked_with != ch1) {
            ch1->linked_with->ref_count--;
        }

        ch1->transfer_to = ch1;
    } else {
        // TODO: decref old linked channels.

        ch1->linked_with = ch2;
        ch2->linked_with = ch1;
        ch1->ref_count++;
        ch2->ref_count++;

    }
}

/// Transfers messages to `src` to `dst`. `src` and `dst` must be in the same
/// process.
void channel_transfer(struct channel *src, struct channel *dst) {
    ASSERT(src->process == dst->process);
    if (src == dst) {
        if (src->transfer_to != src) {
            channel_destroy(src->transfer_to);
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
        receiver->ipc_buffer->header = NOTIFICATION_HEADER;
        receiver->ipc_buffer->from   = 0;
        thread_resume(receiver);
        dst->receiver = NULL;
    }

    return OK;
}
