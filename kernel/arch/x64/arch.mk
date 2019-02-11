CFLAGS += --target=x86_64
CFLAGS += -mcmodel=large -mno-red-zone
CFLAGS += -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2

BOCHS ?= bochs
QEMU ?= qemu-system-x86_64
QEMUFLAGS += -m 256 -nographic -cpu SandyBridge,rdtscp -rtc base=utc
QEMUFLAGS += -device isa-debug-exit,iobase=0xf4,iosize=0x04

kernel_objs += kernel/arch/x64/boot.o
clean += arch/x64/isofiles/kernel.elf kernel.qemu.elf serial.log resea.iso

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
