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

The message buffer to send/receive a message is located in the thread-local
buffer (see [Thread Information Block](#thread-information-block)).

### Notifications
While synchronous (blocking) IPC works pretty fine in most cases, sometimes you
may want asynchronous (non-blocking) IPC. For example, the kernel converts
interrupts into messages but it should not block.

Notifications is a simple solution just like *signal* in Unix:

- `error_t sys_notify(cid_t ch, enum notify_op op, uintmax_t arg0)`
  - Sends a notification to the channel. It simply updates the notification
    field in the destination channel and never blocks (asynchronous).
  - The notification field is an `intmax_t`.

When you receive a message, the kernel first checks the notification. If a
notifcation exists, it returns the notification. Sender threads are blocked
until the notification is received.

#### Notification Operations
`op` specifies how the kernel updates the notification field in `ch`:
- `NOTIFY_OP_SET`: Atomically sets `arg0` to the notification field.
- `NOTIFY_OP_ADD`: Atomically adds `arg0` to the notification field.

Kernel server
-------------
Kernel server is a kernel thread which provdes features that only the kernel can
provide: kernel-level thread (don't get confused with kernel thread!), virtual
memory pager management, interrupts, etc.

Only memmgr and startup servers (apps spawned by memmgr) has a channel
connected to the kernel server. There're no differences between the kernel
server and other servers. Use `sys_ipc` to communicate with the kernel
server as if it is a normal userland server.

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
