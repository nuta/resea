# Porting to a CPU Architecture

Porting to a new CPU architecture (*arch* in short) is threefold:

1. Define arch-specific types and build settings in the `common` library.
2. Implement Hardware Abstraction Layer (HAL) for kernel.
3. Define arch-specific part in the `resea` library.

## Implementing `common` library
The common library (`libs/common`) is responsible for providing libraries and types (`typedef`) for both kernel and userspace programs. You'll need to implement
the following files.

- `libs/common/arch/<arch-name>/arch.mk`
  - Build options for the arch: `$CFLAGS`, `run` command, etc.
- `libs/common/arch/<arch-name>/arch_types.h`
  - Arch-specific `#define`s and `typedef`s. It likely to be changed.

## Porting the kernel
For portability, the kernel separates the arch-specific layer
(*Hardware Abstraction Layer*) into `kernel/arch`.

That said, its interface is not yet stable and undocumented.

Roughly speaking, you'll need to implement:

- CPU initialization
- Debugging log output (e.g. serial port)
- Context switching
- Virtual memory management (updating and switching page tables)
- Interrupt/exception/system call handlers
- Multi-Processor support *(optional)*
- Kernel Debugger input driver *(optional)*
- The linker script for the kernel executable (`kernel/arch/<arch-name>/kernel.ld`)

The x86_64 HAL (`kernel/arch/x64`) would be a good reference.

## Implementing `resea` library
The `resea` library is the standard library for userspace Resea applications.
You'll need to implement:

- The `syscall()` function.
- The entry point of the program: initilize stack pointer, call `resea_init()`,
  call `main()`, and then exit the program in case `main()` returns.
- Bootfs support. The kernel starts the first userspace program embedded in
  the kernel executable as raw binary, not a ELF file. You'll need to handle the
  case when the program is executed from the beginning of `.bootfs` section.
  See `libs/resea/arch/x64/start.S` for a concrete example.
