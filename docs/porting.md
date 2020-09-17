# Porting to a CPU Architecture

Porting to a new CPU architecture (*arch* in short) is pretty easy if you're
already familiar with the architecture:

1. Scaffold your new port using the *example* arch:
  `libs/common/arch/example`, `kernel/arch/example`, and `libs/common/arch/example`.
2. Define arch-specific types and build settings in the `common` library.
3. Implement Hardware Abstraction Layer (HAL) for kernel.
4. Implement arch-specific part in the `resea` library.
5. Add the architecture in `Kconfig`.

## Implementing `common` library
The common library (`libs/common`) is responsible for providing standalone
libraries (e.g. doubly-linked list) and types for both kernel and userspace programs.
You'll need to implement the following files.

- `libs/common/arch/<arch-name>/arch.mk`
  - Build options for the arch: `$CFLAGS`, `run` command, etc.
- `libs/common/arch/<arch-name>/arch_types.h`
  - Arch-specific `#define`s and `typedef`s.

## Porting the kernel
For portability, the kernel separates the arch-specific layer
(*Hardware Abstraction Layer*) into `kernel/arch`.

Roughly speaking, you'll need to implement:

- CPU initialization
- Serial port driver (for `print` functions)
- Context switching
- Virtual memory management (updating and switching page tables)
  - Resea Kernel also supports `NOMMU` mode for CPUs that don't implement virtual memory.
- Interrupt/exception/system call handlers
- The linker script for the kernel executable (`kernel/arch/<arch-name>/kernel.ld`)
- Multi-Processor support *(optional)*

## Implementing `resea` library
The `resea` library is the standard library for userspace Resea applications.
You'll need to implement:

- The `syscall()` function.
- The linker script for userspace programs (`libs/resea/arch/<arch-name>/user.ld`).
- The entry point of the program: initialize stack pointer and then call `resea_init()`.
- Bootfs support. Bootfs is a simple file system image (similar to tar file)
  for Resea. Resea starts the first userspace programs from that file.
  You need to embed the bootfs header to make room for the bootfs header.
  See `libs/resea/arch/x64/start.S` for a concrete example.
