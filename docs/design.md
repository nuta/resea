Design
======

This document describes the design of Resea.

System calls
------------
Similar to *"Everything is a file"* philosophy in Unix, Resea has a philosophy:
*"Everything is a message passing"*. Want to read a file? Send a message to
the file system server! Want to spawn a new thread? Send a message to the kernel
server!

All system calls are only essential ones for message passing:

- `int sys_open(void)`
  - Creates a new channel.
- `error_t sys_close(cid_t ch)`
  - Destroys a channel.
- `error_t sys_ipc(cid_t ch, uint32_t ops)`
  - Performs asynchronous IPC operations. `ops` can be send, receive, or both
    of them (so-called *Remote Procedure Call*).
- `error_t sys_link(cid_t ch1, cid_t ch2)`
  - Links two channels. Messages from `ch1` (resp. `ch2`) will be sent to `ch2`
    (resp. `ch1`).
- `error_t sys_transfer(cid_t src, cid_t dst)`
  - Transfer messages from the channel linked to `src` to `dst` channels.

### Message Structure
You may noticed that `sys_ipc` does not take a pointer to the message buffer
to send/receive: the message buffer is located in the thread-local buffer. It
simplifies and optimizes the IPC implementation in kernel a bit.

The message layout is as follows:

```c
struct message {
    header_t header;
    cid_t from;
    notification_t notification;
    cid_t channel;
    page_t page;
    uint8_t padding[/* Depends on arch. */];
    uint8_t data[INLINE_PAYLOAD_LEN_MAX];
};
```

#### Payloads
- *Inline Payload* is a small (less than about 200 bytes) arbitrary data that
  simplified copied to the destination thread's IPC buffer.
- *Channel Payload* delegates the specified sender's channel to the destination
  process.
- *Page Payload* transfers physical memory pages to the destination process.
  - The kernel accepts this payload only if the receiver thread sets the
    page base in its Thread Information Block.
  - This payload *moves* the physical pages. Sent pages are no longer
    accessible in the sender process. This implies that it is impossible to
    implement *Shared Memory* in Resea.



### Notifications
While synchronous (blocking) IPC works pretty fine in most cases, sometimes you
may want asynchronous (non-blocking) IPC. For example, the kernel converts
interrupts into messages but it should not block.

Notifications is a simple solution just like *signal* in Unix:

- `error_t sys_notify(cid_t cid, notification_t notification)`
  - Sends a notification to the channel. It simply updates the notification
    field in the destination channel (by OR'ing `notification`).
  - If a receiver thread already waits for a message on the destination channel,
    the kernel sends a message as `NOTIFICATION_MSG` to it.
  -  It never blocks (asynchronous).
  - `notification_t` is `uint32_t`.

Kernel server
-------------
Kernel server is a kernel thread which provdes features that only the kernel can
provide: kernel-level thread (don't get confused with kernel thread!), virtual
memory pager management, interrupts, etc.

System Call Data Structures
----------------------------

### System Call ID
```
|31                                      16|15        8|7                   0|
+------------------------------------------+-----------+---------------------+
|                (reserved)                |    ops    |   syscall number    |
+------------------------------------------+-----------+---------------------+
```

- `ops` is `ipc`-system-call-specific field. It specifies the IPC operations
  and its options.

### Message Header
```
|31                 16|15                                 |12|11|10         0|
+---------------------+-----------------------------------+--+--+------------+
|         type        |                (reserved)         |ch|pg| inline len |
+---------------------+-----------------------------------+--+--+------------+
```

- `type`: The message type (e.g. `spawn_thread`).
- `pg`: If set, a page payload is contained.
- `ch`: If set, a channel payload is contained.

### Page Payload
```
|63                                         12|11                 5|4       0|
+---------------------------------------------+------------------------------+
|             page address (vaddr >> 12)      |     (reserved)     |   exp   |
+---------------------------------------------+------------------------------+
```

- The number of pages are computed as `pow(2, exp)`, that is, the number of
  pages must be a power of the two.
