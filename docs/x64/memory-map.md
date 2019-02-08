Memory Map
==========

## Physical Address Space

| Physical Address         | Description                                                            | Length              |
| ------------------------ | ---------------------------------------------------------------------- | ------------------- |
| `0000_0000`              | BIOS / Video Memory / ROM                                              | 1MiB                |
| `0010_0000`              | Kernel Executable                                                      | up to 6MiB          |
| `0070_0000`              | Page table for the kernel space (maps `ffff_8000_0000_0000` or higher) | 3MiB                |
| `00a0_0000`              | CPU-local boot / idle thread stack                                     | 1MiB (4kiB per CPU) |
| `00b0_0000`              | CPU-local variables (`struct CPU`)                                     | 1MiB                |
| `00d0_0000`              | Unused                                                                 | 3MiB                |
| `0100_0000`              | Kernel Data (`AllocationPool` for small objects like `struct Thread`)  | 4MiB                |
| `0140_0000`              | Kernel Data (`AllocationPool` for kernel stacks)                       | 4MiB                |
| `0180_0000`              | Kernel Data (`AllocationPool` for page-sized objects)                  | 8MiB                |
| `0200_0000`              | Available for userspace                                                |                     |
| `fee0_0000`              | Local APIC                                                             |                     |
| to the limit of memory   | Available for userspace                                                |                     |

## Virtual Address Space

| Virtual Address          | Mapped to (Physical Address)                          | Description  | Length                        |
| ------------------------ | ----------------------------------------------------- | ------------ | ----------------------------- |
| `0000_0000_0000_0000`    | Managed by userspace programs                         | User space   | the limit of CPU architecture |
| `ffff_8000_0000_0000`    | `0000_0000_0000_0000` (so-called straight mapping)    | Kernel space | 4GiB                          |

### Remarks
- The kernel space simply maps 4GiB pages to access the memory-mapped Local APIC registers: the kernel does not use
  `[ffff_8000_0200_0000, ffff_8000_fee0_0000)`.