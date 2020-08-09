# Porting to a CPU Architecture

Porting to a new CPU architecture (*arch* in short) is pretty easy if you're
already familiar with the architecture:

1. Scaffold your new port using the *example* arch:
  `libs/common/arch/example`, `kernel/arch/example`, and `libs/common/arch/example`.
2. Define arch-specific types and build settings in the `common` library.
3. Implement Hardware Abstraction Layer (HAL) for kernel.
4. Define arch-specific part in the `resea` library.
5. Add the architecture in `Kconfig`.

## Implementing `common` library
The common library (`libs/common`) is responsible for providing libraries and types (`typedef`) for both kernel and userspace programs. You'll need to implement
the following files.

- `libs/common/arch/<arch-name>/arch.mk`
  - Build options for the arch: `$CFLAGS`, `run` command, etc.
- `libs/common/arch/<arch-name>/arch_types.h`
  - Arch-specific `#define`s and `typedef`s.

## Porting the kernel
For portability, the kernel separates the arch-specific layer
(*Hardware Abstraction Layer*) into `kernel/arch`.

Roughly speaking, you'll need to implement:

- CPU initialization
- Debugging log output (e.g. serial port)
- Context switching
- Virtual memory management (updating and switching page tables)
  - Resea Kernel also supports `NOMMU` mode for CPUs that don't implement virtual memory.
- Interrupt/exception/system call handlers
- Multi-Processor support *(optional)*
- Kernel Debugger input driver *(optional)*
- The linker script for the kernel executable (`kernel/arch/<arch-name>/kernel.ld`)

## Implementing `resea` library
The `resea` library is the standard library for userspace Resea applications.
You'll need to implement:

- The `syscall()` function.
- The entry point of the program: initialize stack pointer, call `resea_init()`,
  call `main()`, and then exit the program in case `main()` returns.
- Bootfs support. The kernel starts the first userspace program embedded in
  the kernel executable as raw binary, not a ELF file. You'll need to handle the
  case when the program is executed from the beginning of `.bootfs` section.
  See `libs/resea/arch/x64/start.S` for a concrete example.
