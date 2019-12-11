# Message Passing (IPC)
IPC on Resea is channel-based and synchronous.

## Message Structure
You may noticed that `ipc` system call does not take a pointer to the message buffer
to send and receive messages: the message buffer is located in the thread-local buffer. It
simplifies and optimizes the IPC implementation in the kernel a bit.

A message is consisted of the following fields:

- Message Header
  - Type of message
  - Length of the inline payload
  - Whether the page payload is set
  - Whether the channel payload is set
- Source channel ID (set by the kernel)
- Notification
- Channel Payload
- Page Payload
- Inline Payload

### Payloads
- *Inline Payload* is small (less than about 200 bytes) arbitrary data that
  simply copied to the destination thread's IPC buffer.
- *Channel Payload* delegates the specified sender's channel to the destination
  process.
- *Page Payload* transfers physical memory pages to the destination process.
  - The kernel accepts this payload only if the receiver thread sets the
    page base in its Thread Information Block.
  - This payload *moves* the physical pages. Sent pages are no longer
    accessible in the sender process. This implies that it is impossible to
    implement *Shared Memory* in Resea.


## IPC Fastpath
Resea Kernel implements *IPC fastpath*, an optimized IPC implementation which
would be faster than the full-featured IPC implementation.

The kernel automatically switches to the fastpath mode if some conditions are met including:
- Payloads are inline only (i.e., no channel/page payloads).
- Both send and receive operations are specified.
- A receiver thread already waits for a message.
