# Change Log

## v0.3.0 (Aug 2, 2020)
- Reorganized system calls into: `exec`, `ipc`, `listen`, `map`, `print`, and `kdebug`.
- Removed kernel heap for separation of mechanism and policy. Task data structures
  are now statically allocated in the kernel's `.data` section, and page table
  structures are now allocated from the userland (through `kpage` parameter in `map` system call).
- Started implementing Rust support. Currently, there's only a "Hello World"
  sample app. I'll add APIs once the C API gets stabilized (hopefully September).
- shell: Add `log` command to print the kernel log.
- Many bug fixes and other improvements.

## v0.2.0 (June 14, 2020)
- Add experimental support for [Micro:bit](https://microbit.org) (ARMv6-M).
- Add experimental support for [Raspberry Pi 3](https://www.raspberrypi.org/products/raspberry-pi-3-model-b-plus/) (AArch64 in ARMv8-A).
- Add *experimental (still in the early stage)* Linux ABI emulation layer: run your Linux binary *as it is* on Resea!
- A new build system.
- Bunch of breaking changes, bug fixes, and improvements.
