# IPC / System Call Data Structures
This page describes data structures that are used by both the kernel and userland
programs.

## System Call ID
```
|31                                      16|15        8|7                   0|
+------------------------------------------+-----------+---------------------+
|                (reserved)                |    ops    |   syscall number    |
+------------------------------------------+-----------+---------------------+
```

- `ops` is `ipc` system call specific field. It specifies the IPC operations
  and its options. Avialble flags are:
  - `IPC_SEND`: Perform a send operation.
  - `IPC_RECV`: Perform a recv operation.
  - `IPC_NOBLOCK`: The system call returns an error if the specified operation
     would block.
- If both `IPC_SEND` and `IPC_RECV` are set, the kernel first performs the
  send operations.

## Message Header
```
|31                 16|15                                 |12|11|10         0|
+---------------------+-----------------------------------+--+--+------------+
|         type        |                (reserved)         |ch|pg| inline len |
+---------------------+-----------------------------------+--+--+------------+
```

- `type`: The message type (e.g. `kernel.spawn_thread`).
- `pg`: If set, the message contains a page payload.
- `ch`: If set, the message contains a channel payload.

## Page Payload
```
|63                                         12|11                           0|
+---------------------------------------------+------------------------------+
|             page address (vaddr >> 12)      |   (reserved: should be 0)    |
+---------------------------------------------+------------------------------+
|                                length in bytes                             |
+----------------------------------------------------------------------------+
```

- The length need not be aligned to the page size.
- The number of pages is computed as `ALIGN_UP(page_len, PAGE_SIZE) / PAGE_SIZE`.


### Thread Infomation Block
Thread Information Block (TIB) is a thread-local page linked by the kernel. TIB
is always visible from both the kernel and the user. In x64, you can locate it
by the RDGSBASE instruction in the user mode.

```
|63                                                                         0|
+----------------------------------------------------------------------------+
|                               arg  (user-defined)                          |  0
+----------------------------------------------------------------------------+
|                                Page Payload Base                           |  8
+----------------------------------------------------------------------------+
|                                                                            |  16
|                                                                            |
|                                  IPC buffer                                |
|                               (struct message)                             |
|                                                                            |
+----------------------------------------------------------------------------+
```
