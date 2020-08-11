# IPC

## Header File
```c
#include <resea/ipc.h>
```

## Message Structure
A message is fixed-sized. It contains the message type (or an error), the sender
task ID, and the message payload (arbitrary bytes, defined by [IDL](idl)).

```c
/// Message.
struct message {
    /// The type of message. If it's negative, this field represents an error
    /// (error_t).
    int type;
    /// The sender task of this message.
    task_t src;
    /// The message contents. Note that it's a union, not struct!
    union {
        // The message contents as raw bytes.
        uint8_t raw[MESSAGE_SIZE - sizeof(int) - sizeof(task_t)];

        // The common header of message fields.
        struct {
            /// The ool pointer to be sent. Used if MSG_OOL is set.
            void *ool_ptr;
            /// The size of ool payload in bytes.
            size_t ool_len;
        };

        // Auto-generated message fields:
        //
        //     struct { notifcations_t data; } notifcations;
        //     struct { task_t task; ... } page_fault;
        //     struct { paddr_t paddr; } page_reply_fault;
        //     ...
        //
        IDL_MESSAGE_FIELDS
   };
};
```

## Sending a Message
In Resea, IPC operations are sychronous. The destination is specified
by a task ID. For simplicity, we don't provide indirect IPC mechanism so-called
*channel*.

```c
error_t ipc_send(task_t dst, struct message *m);
error_t ipc_send_err(task_t dst, error_t error);
error_t ipc_send_noblock(task_t dst, struct message *m);
```

`ipc_send_err` is a wrapper function which sets `error` to `m.type` and then
sends the error message.

`ipc_send_noblock` tries to send a message. If the desitnation task is not
ready for receiving a message, it immediately returns `ERR_WOULD_BLOCK`
instead of blocking the sender task.

## Receiving a Message
On a receive operation, you have two options, *open receive* and *closed receive*:

- *open receive* (when `src == IPC_ANY`): accepts a message from any tasks.
- *closed receive* (otherwise): accepts a message from the specific task (`src`). Other sender tasks are blocked.


```c
error_t ipc_recv(task_t src, struct message *m);
```

## Replying a Message from a Server
In case the sender task does not wait for a reply message, use the following
wrapper functions (they wrap `ipc_send_noblock`). If the client calls the server with
`ipc_call`, these APIs should success.

```c
void ipc_reply(task_t dst, struct message *m);
void ipc_reply_err(task_t dst, error_t error);
```

## Sending Notifications
*Notifications* is a asynchronous IPC like *signals* in UNIX. Each task has its
own notifications bitfield. When a task tries to receive a meesage and pending
notifications exist (i.e. the bitfield is not zero), the kernel constructs and
returns `NOTIFICATIONS_MSG` with the notifications.

```c
error_t ipc_notify(task_t dst, notifications_t notifications);
```

`ipc_notify` does bitwise-OR operation on the destination task's
notifications bitfield and the given bits, i.e. `dst->notifications |= notifications`.

## Send and Receive a Message at once
```c
error_t ipc_call(task_t dst, struct message *m);
error_t ipc_replyrecv(task_t dst, struct message *m);
```

`ipc_call` is same as `ipc_send(dst, m)` and then `ipc_recv(dst, m)`. Clients
should use this API instead of calling those two APIs or `ipc_reply` from the
server may fail.

Both APIs overwrite the message buffer `m` with the received message.

`ipc_replyrecv` is same as `ipc_reply(dst, m)` and then `ipc_recv(IPC_ANY, m)`. With this API, you can reduce the number of system calls in the server.
