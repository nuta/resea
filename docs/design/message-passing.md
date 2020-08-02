# Message Passing
The message passing is the only way provided by the kernel to communicate
with other tasks.

The IPC operations, *sending* and *receiving* a message, are
synchronous (i.e. blocking). The desitnation of a message is specified by task ID
and Resea does not provide the notion of *channel* or *port* of IPC for simplicity.

## Why not asynchronous?
TODO: Synchronous IPC is good for performance and for the separation of mechanism
and policy. Survey L4 microkernels if you're interested in.

## Message

A message is fixed sized and consists of the following fields:

- The type of message (e.g. `FS_READ_MSG`).
- The sender task ID.
- The fixed-sized payload (less than 256 bytes).
  - Resea supports an indirect arbitrary-length data copy called *out-of-line payload* (*"ool"* in short) .
    It is mainly used for sending/receiving buffer (a pair of the pointer and
    the length).

## Message Passing APIs

- `error_t ipc_send(task_t dst, struct message *m);`
    - Sends a message. It waits the receiver task.
- `error_t ipc_send_noblock(task_t dst, struct message *m);`
    - Try seding a message. If it would block, it returns.
- `error_t ipc_send_err(task_t dst, error_t error);`
    - Sends an error message.
- `void ipc_reply(task_t dst, struct message *m);`
    - Same as `ipc_send_noblock`. It is typically used to emphasize that the
      receiver shall already waiting for a message.
- `void ipc_reply_err(task_t dst, error_t error);`
    - Same as `ipc_send_err`.
- `error_t ipc_recv(task_t src, struct message *m);`
    - Receive a message from the task specified in `src`. If `src` is zero,
      it waits for a message from arbitrary tasks
      (so-called *[open receive](http://www.cse.unsw.edu.au/~cs9242/07/lectures/02-l4.pdf)*).
- `error_t ipc_call(task_t dst, struct message *m);`
    - Send a message to `dst` and then receive a message from the destination
      task. You must use this API if the destination task replies a response
      by `ipc_reply`.
- `error_t ipc_notify(task_t dst, notifications_t notifications);`
    - Send a notification (see *Notifications* section).
