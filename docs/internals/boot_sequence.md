# Boot Sequence
1. The bootloader (e.g. GRUB) loads the kernel image.
2. Arch-specific boot code initializes the CPU and essential peripherals.
3. Kernel initializes subsystems: debugging, memory, process, thread, etc. ([kernel/boot.c](https://github.com/nuta/resea/blob/master/kernel/boot.c))
4. Kernel creates the very first userland process from initfs.
5. The first userland process ([memmgr](../servers/memmgr)) spawns servers from initfs.
