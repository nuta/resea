#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <types.h>
#include <message.h>
#include <arch.h>
#include <list.h>

struct process;

/// The channel control block.
struct channel {
    /// The owner process.
    struct process *process;
    /// The channel lock.
    spinlock_t lock;
    /// The channel ID.
    cid_t cid;
    /// The reference counter. Even if the owner process has been destroyed,
    /// some of its channels must be alive to prevent dangling pointers until
    /// no one references them.
    int ref_count;
    /// The received notification data.
    notification_t notification;
    /// The channel linked with this channel. The messages sent from this
    /// channel will be sent to this channel. The destination channel can be
    /// channels in another process.
    ///
    /// By default, this points to the itself (i.e. `ch->linked_with == ch`).
    struct channel *linked_with;
    /// Messages are transferred to the this channel. Consider the following
    /// example:
    ///
    ///    +-- A --+               +------------ B ----------+
    ///    |       |  linked_with  |       transfered_to     |
    ///    |  @7 <-------------------> @2  =============> @3 |
    ///    |       |               |                         |
    ///    +-------+               +-------------------------+
    ///
    /// When A sends a message to her channel @7, the kernel sets the `from`
    /// field in the message to the channel ID linked with @7, i.e., @2. Next,
    /// it sends the message to the channel specified in the `transfer_to` field
    /// in @2', namely, @3.
    ///
    /// By default, this points to the itself (i.e. `ch->transfer_to == ch`).
    struct channel *transfer_to;
    /// The receiver thread. This is NULL if no threads are in the receive
    /// phase on this channel.
    struct thread *receiver;
    /// The sender thread queue.
    struct list_head queue;
};

struct channel *channel_create(struct process *process);
void channel_incref(struct channel *ch);
void channel_destroy(struct channel *ch);
void channel_link(struct channel *ch1, struct channel *ch2);
void channel_transfer(struct channel *src, struct channel *dst);
error_t channel_notify(struct channel *ch, notification_t notification);

#endif
