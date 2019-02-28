CFLAGS += --target=x86_64
CFLAGS += -mcmodel=large -mno-red-zone
CFLAGS += -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2

BOCHS ?= bochs
QEMU ?= qemu-system-x86_64
QEMUFLAGS += -m 256 -nographic -cpu SandyBridge,rdtscp -rtc base=utc
QEMUFLAGS += -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=0x04

kernel_objs += kernel/arch/x64/boot.o
clean += arch/x64/isofiles/kernel.elf kernel.qemu.elf serial.log resea.iso

kernel_objs += \
	kernel/arch/x64/apic.o \
	kernel/arch/x64/cpu.o \
	kernel/arch/x64/exception.o \
	kernel/arch/x64/gdt.o \
	kernel/arch/x64/handler.o \
	kernel/arch/x64/idt.o \
	kernel/arch/x64/interrupt.o \
	kernel/arch/x64/ioapic.o \
	kernel/arch/x64/paging.o \
	kernel/arch/x64/pic.o \
	kernel/arch/x64/print.o \
	kernel/arch/x64/setup.o \
	kernel/arch/x64/smp.o \
	kernel/arch/x64/switch.o \
	kernel/arch/x64/tss.o \
	kernel/arch/x64/thread.o \
	kernel/arch/x64/spinlock.o \
	kernel/arch/x64/string.o \
	kernel/arch/x64/syscall.o

bochs: resea.iso
	$(BOCHS) -qf boot/x64/bochsrc

run: kernel.qemu.elf
	$(PROGRESS) QEMU kernel.qemu.elf
	$(PYTHON3) ./tools/run-qemu.py -- $(QEMU) $(QEMUFLAGS) -kernel kernel.qemu.elf

kernel.qemu.elf: kernel.elf
	$(PROGRESS) TWEAK $@
	cp kernel.elf $@
	$(PYTHON3) ./tools/tweak-elf-for-qemu.py $@
	$(PROGRESS) QEMU $@

resea.iso: kernel.elf
	cp kernel.elf boot/x64/isofiles/kernel.elf
	$(PROGRESS) GRUB $@
	grub-mkrescue -o $@ boot/x64/isofiles
