# Out-of-Line Payload
Since a message is fixed-sized and the size is very small (typically 256 bytes), we need another way to send large data (e.g. file contents).

*Out-of-Line payload* (*OoL* in short) is a feature implemented by `vm` server for that purpose.

## OoL Types

OoL supports the following payload types:

| Type    | Description       |
|---------|-------------------|
| `bytes` | an arbitrary data |
| `str`   | a string terminated with `\0` |

## Caveats
- It's slow for now since it needs some IPC calls with `vm`.
- Only single OoL payload is supported per message.
- The maximum size of an OoL payload is configureable in the build config.

## Sending a OoL Payload
OoL is integrated with the IDL and userspace library. Let's take a look at an example:

```
namespace fs {
    rpc open(path: str) -> (handle: handle);
    rpc create(path: str, exist_ok: bool) -> (handle: handle);
    rpc close(handle: handle) -> ();
    rpc read(handle: handle, offset: offset, len: size) -> (data: bytes);
    rpc write(handle: handle, offset: offset, data: bytes) -> ();
    rpc stat(path: str) -> (size: size);
}
```

In the `fs` interface definition shown above, as you can see, some methods use OoL payloads. `fs.open` uses `str` payload to send a path name to be opened, and `fs.read` returns a `bytes` payload for the read file contents.


To send a `bytes`, set a pointer to data and the length of data:

```c
static void reply_file_contents(task_t client, uint8_t *data, size_t len) {
    struct message m;
    m.type = FS_READ_REPLY_MSG;
    m.fs_read_reply.data = data;
    m.fs_read_reply.data_len = len;
    ipc_call(client, &m);
}
```


To send a `str`, set a pointer to a *null-terminated* ASCII string to as `str` payload. The IPC library computes the length of the OoL payload by `strlen` and transparently copies it into the destination task (fs server):

```c
struct message m;
m.type = FS_OPEN_MSG;
m.fs_open.path = "/hello.txt";
error_t err = ipc_call(fs_server, &m);
```

## Receiving an OoL Payload
In the fs server, the IPC library sets a valid pointer to the OoL payload field. For `str` payloads, it is guaranteed that the string is terminalted by `\0`.

```c
struct message m;
ipc_recv(IPC_ANY, &m);
if (m.type == FS_OPEN_MSG) {
    DBG("path = %s", m.fs_open.path);
}
```

For `bytes` payload, use `<name>_len` to determine the size of the payload:

```c
ipc_recv(fs_server, &m);
if (m.type == FS_READ_REPLY_MSG) {
    HEXDUMP(m.fs_read_reply.data, m.fs_read_reply.data_len);
}
```

A memory buffer for received OoL payload is dynamically allocated by `malloc`. **Don't forget to `free`!**

## How It Works
```
+--------+  3. ool.send  +------+  2. ool.recv  +----------+
| sender |  -----------> |  vm  | <------------ | receiver |
|  task  |               |      |               |   task   |
|        |               |      |  4. copy OoL  |          |
|        |               |      | ------------> |          |
|        |               |      |               |          |
|        |               |      | 5. send msg   |          |
|        | -----------------------------------> |          |
|        |               |      |               |          |
|        |               |      | 6. ool.verify |          |
|        |               |      | <------------ |          |
+--------+               +------+               +----------+
```

1. For messages with a ool payload, IPC stub generator adds `MSG_OOL` to the message type field (i.e. `(m.type & MSG_OOL) != 0` is true).
2. In `ipc_recv` API, the receiver task sends a `ool.recv` message to tell the location of the OoL receive buffer (allocated by `malloc`) to the vm server.
3. When a sender task `ipc_send` API, if `MSG_OOL` bit is set, it calls `ool.send` to the vm server before sending the message.
4. The vm server looks for an unsed OoL buffer in the desitnation task, copies the OoL payload into the buffer, and returns the pointer to buffer in the receiver's address space.
5. The sender tasks overwrites the OoL field with the receiver's pointer and sends the message.
6. Once receiver task received a message with OoL, it calls vm's `ool.verify` to check if the received pointer and the length is valid.
7. `ipc_recv` returns.

### Why not Implement OoL in Kernel?
In fact, OoL is initially implemented in the kernel and is removed later because it turned out that page fault handling makes the kernel complicated.

Since we can now map memory pages in the userspace through the `map` system call, I believe that it is a better idea to implement a more efficient message passing with OoL support within userspace using shared memory.
