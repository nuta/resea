Internals
=========

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
| `0000_0000_0340_0000`    | memmgr .bss (initfs.bin doesn't contain .bss section) | 12MiB            |
| `0000_0000_0400_0000`    | Mapped to physical pages (straight mapping)           | the limit of RAM |

### Apps spawned by memmgr (x64)

| Virtual Address          | Description                                           | Length       |
| ------------------------ | ----------------------------------------------------- | ------------ |
| `0000_0000_00f1_b000`    | Thread Information Block                              | 4KiB         |
| `0000_0000_1000_0000`    | Executable (.text, .rodata, .data, and .bss)          | 16MiB        |
| `0000_0000_2000_0000`    | Stack                                                 | 16MiB        |
| `0000_0000_3000_0000`    | Heap                                                  | 16MiB        |
