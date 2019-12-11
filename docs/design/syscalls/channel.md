# Channel
*Channel* is an endpoint of IPC just like *sockets* in Unix. A channel is owned
by a process and is accessible from all threads in the process.

## Creating and Closing a Channel
The `open` and `close` system call creates and destructs a channel respectively.

## Linking
Each channel is linked to the destination channel. We denote linked channels
`A` and `B` as `A <-> B`. Those channel owners can be different processes.
By default, a newly created channel is linked to itself (namely `A <-> A`).

Performing an IPC send operation on the channel `A` transmits a message
(in the sender thread's IPC buffer) to the thread which waits for a message
on the channel `B`.

The `link` system call allows updating linking settings.

## Transfering
Each channel has an optional setting to transfer messages to another channel.
We denote it as `A => B`. Consider the following example:

If channels are setted up as `A <-> B => C => D`, when sending a message on `A`,
the kernel uses `C` as the destination channel instead of `B`. The kernel does
not follow the transferring setting in `C` in order to prevent an infinite loop
caused by circular references.

Note that transfering is one-way: if you send a message on `C`, the destination
channel is not `A`.

The `transfer` system call allows updating transfering settings.
