# Message Passing
The message passing is the only way provided by the kernel to communicate
with other tasks.

The IPC operations, *sending* and *receiving* a message, are
synchronous (i.e. blocking). The desitnation of a message is specified by task ID
and Resea does not provide the notion of *channel* or *port* of IPC for simplicity.

For more details, see [IPC API](../userspace/ipc).

## Why not asynchronous?
Synchronous IPC is good for performance and for the separation of mechanism
and policy. Survey L4 microkernels if you're interested in.

TODO: Add references.

## Message

A message is fixed sized and consists of the following fields:

- The type of message (e.g. `FS_READ_MSG`).
- The sender task ID.
- The fixed-sized payload (less than 256 bytes, depends on arch).

We use our own [Interface Definition Language](../userspace/idl/) to generate message definitions.
