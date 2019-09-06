Internals
=========

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
*The detailed design is under consideration and this feature is not yet implemented.* 

While synchronous (blocking) IPC works pretty fine in most cases, sometimes you
may want asynchronous (non-blocking) IPC. For example, the kernel converts
interrupts into messages but it should not block.

Notifications is a simple solution just like *signal* in Unix:

1. The `notify` system call sends a notification. It simply updates the
   notification field in the destination channel and never blocks
   (asynchronous).
2. When you receive a message from the channel, the kernel first checks the
   notification field. If a notifcation exists, it returns the notification.

Kernel server
-------------
Kernel server is a kernel thread which provdes features that only the kernel can
provide: kernel-level thread (do't get confused with kernel thread!), virtual
memory pager management, interrupts, etc.

Only memmgr and startup servers (apps spawned by memmgr) has a channel
connected to the kernel server at cid 1. There're no differences between the
kernel server and other servers. Use `sys_ipc` to communicate with the kernel
server as if it is a normal userland server.

Data structures
---------------

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
|31                 16|15           14|13        12|11        11|10         0|
+---------------------+---------------+------------+------------+------------+
|         type        | # of channels | # of pages | (reserved) | inline len |
+---------------------+---------------+------------+------------+------------+
```

- `type`: The message type (e.g. `spawn_thread`).

### Page Payload
```
|63                                         12|11         7|6     5|4       0|
+---------------------------------------------+------------+-------+---------+
|             page address (vaddr >> 12)      | (reserved) | type  |   exp   |
+---------------------------------------------+------------+-------+---------+
```

- There two page types: `move` and `shared`.
  - For a `move` page type, the kernel unlinks the page from the sender's address
    space and links it to the receiver's address space. That is, the page is no
    longer accessible in the sender.
  - For a `shared` page type, As the name implies, it implements a sort of shared memory.
    The kernel keeps the page in the sender's address space and links it to the
    receiver's address space. That is, both sender and receiver can read and
    write the same physical page.
- The number of pages are computed as `pow(2, exp)`. The number of pages
  must be a power of the two.

### Thread Infomation Block
Thread Information Block (TIB) is a thread-local page filled by the kernel. TIB
is always visible from both the kernel and the user. In x64, you can locate it
by the RDGSBASE instruction in the user mode.

```
|63                                                                         0|
+----------------------------------------------------------------------------+
|                               arg  (user-defined)                          |  0
+----------------------------------------------------------------------------+
|                                                                            |  8
|                                                                            |
|                                  IPC buffer                                |
|                               (struct message)                             |
|                                                                            |
+----------------------------------------------------------------------------+
```

Boot sequences
--------------
1. The bootloader (e.g. GRUB) loads the kernel image.
2. Arch-specific boot code initializes the CPU and essential peripherals.
3. Kernel initializes subsystems: debugging, memory, process, thread, etc. ([kernel/boot.c](https://github.com/seiyanuta/resea/blob/master/kernel/boot.c))
4. Kernel creates the very first userland process from initfs.
5. The first userland process (typically [memmgr](https://github.com/seiyanuta/resea/tree/master/apps/memmgr))
   spawns servers from initfs.

Initfs
------
Initfs is a simple file system embedded in the kernel executable. It contains
the first userland process image (typically memmgr) and some essential servers
to boot the system. It's equivalent to *initramfs* in Linux.

Note that kernel does not parse it at all. It simply maps into the specific
address space to avoid adding the code to kernel as possible.

Memory maps
-----------

### Physical address space (x64)

| Physical Address         | Description                                                            | Length              |
| ------------------------ | ---------------------------------------------------------------------- | ------------------- |
| `0000_0000`              | BIOS / Video Memory / ROM                                              | 1MiB                |
| `0010_0000`              | Kernel Executable                                                      | up to 6MiB          |
| `0070_0000`              | Page table for the kernel space (maps `ffff_8000_0000_0000` or higher) | 3MiB                |
| `00a0_0000`              | CPU-local boot / idle thread stack                                     | 1MiB (4kiB per CPU) |
| `00b0_0000`              | CPU-local variables (`struct CPU`)                                     | 1MiB                |
| `00d0_0000`              | Unused                                                                 | 3MiB                |
| `0100_0000`              | Kernel Data (`struct arena` for small objects)                         | 4MiB                |
| `0140_0000`              | Kernel Data (`struct arena` for page-sized objects)                    | 12MiB               |
| `0300_0000`              | Available for userspace (managed by memmgr)                            |                     |
| `1000_0000`              | Kernel Address Sanitizer shadow memory (TODO: move to a better place)  | 256MiB              |
| `fee0_0000`              | Local APIC                                                             |                     |
| to the limit of RAM      | Available for userspace (managed by memmgr)                            |                     |

### Virtual address space (x64)

| Virtual Address          | Mapped to (Physical Address)                          | Description  | Length                        |
| ------------------------ | ----------------------------------------------------- | ------------ | ----------------------------- |
| `0000_0000_0000_0000`    | Managed by userspace programs                         | User space   | the limit of CPU architecture |
| `ffff_8000_0000_0000`    | `0000_0000_0000_0000` (so-called straight mapping)    | Kernel space | 4GiB for now                  |

- The kernel space simply maps 4GiB pages to access memory-mapped Local APIC registers: the kernel does not use
  `[ffff_8000_0200_0000, ffff_8000_fee0_0000)`.

### Memmgr (x64)

| Virtual Address          | Description                                           | Length           |
| ------------------------ | ----------------------------------------------------- | ---------------- |
| `0000_0000_00f1_b000`    | Thread Information Block                              | 4KiB             |
| `0000_0000_0100_0000`    | initfs.bin (.text, .rodata, .data)                    | 16MiB            |
| `0000_0000_0300_0000`    | memmgr stack                                          | 4MiB             |
| `0000_0000_0340_0000`    | memmgr .bss (initfs.bin doesn't include .bss section) | 12MiB            |
| `0000_0000_0400_0000`    | Mapped to physical pages (straight mapping)           | the limit of RAM |

### Apps spawned by memmgr (x64)

| Virtual Address          | Description                                           | Length       |
| ------------------------ | ----------------------------------------------------- | ------------ |
| `0000_0000_00f1_b000`    | Thread Information Block                              | 4KiB         |
| `0000_0000_1000_0000`    | Executable (.text, .rodata, .data, and .bss)          | 16MiB        |
| `0000_0000_2000_0000`    | Stack                                                 | 16MiB        |
| `0000_0000_3000_0000`    | Heap                                                  | 16MiB        |
