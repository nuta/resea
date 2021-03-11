# Change Log

## v0.9.0 (Mar 11, 2021)
- Shared memory API (by @arpitvaghela)
- Add virtio-gpu driver
- Rust: Support `alloc` crate
- Some bug fixes and trivial improvements

## v0.8.0 (Dec 24, 2020)
- x64: Hardware-assisted hypervisor support (like Linux KVM)
- Add hypervisor implementation [hv](https://resea.org/docs/servers/hv.html)
- Add (*not* cryptographic-secure) random number generator server (by @malbx)
- x64: Support printing messages on the screen
- vm: Support the physical memory map from GRUB
- Fix a typo in docs (by @JeetKaria06)
- Add link to IRC channel (by @milisarge)

## v0.7.0 (Nov 22, 2020)
- Add the datetime server with UNIX timestamp support (by @PrayagS)
- Add the x64-based system's RTC device driver (by @PrayagS)
- Mitigate malloc heap fragmentation (by @yashrajkakkad)
- Add Intel HD Audio device driver
- vm: Fix memory leaks
- genidl: Support `--lang html`
- raspi3: Initial (incomplete) support for multiprocessors
- raspi3: Enable memory cache
- benchmark: Some improvements for arm64

## v0.6.0 (Oct 30, 2020)
- Add [unit testing framework](https://resea.org/docs/userspace/unit-test.html) (experimental).
- Add the ATA (PIO mode) disk driver (`servers/drivers/blk/ide`).
- fatfs: Support writing into a file.
- tcpip: Implement the DNS client.
- kernel: Isolate machine-specific code.
- kernel/x64: Mitigate the CPU high usage caused by timer interrupts.
- Reorganize and refactor IDL definitions.

## v0.5.0 (Oct 3, 2020)
- Support bare-metal Raspberry Pi 3B+. Resea now boots on *real* Raspberry Pi!
- Support Google Compute Engine: A HTTP server (`servers/apps/webapi`) on Resea works in the cloud!
- Add the virtio-net device driver. It supports both modern and legacy devices.
- tcpip: Support sending ICMP echo request.
- tcpip: Fix some bugs in the DHCP client.
- Some other bug fixes and improvements.

## v0.4.0 (Aug 21, 2020)
- shell: Use the serial port driver in kernel for the shell access.
- Support command-line arguments.
- libs/resea: Add parsing library `<resea/cmdline.h>`.
- tcpip: Implement TCP active open.
- Add command-line utilities application named `utils`.
- Remove `display` and `ps2kbd` device drivers.
- kernel: Deny kernel memory access from the userspace by default.
- kernel: Reorganize internal interfaces.
- Introduce [sparse](https://www.kernel.org/doc/html/v4.12/dev-tools/sparse.html), a static analyzer for C.

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
