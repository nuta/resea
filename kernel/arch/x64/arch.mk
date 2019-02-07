override CFLAGS += -O2 -g3 --target=x86_64
override CFLAGS += -ffreestanding -fno-builtin -nostdlib -mcmodel=large
override CFLAGS += -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2

BOCHS ?= bochs
QEMU ?= qemu-system-x86_64
QEMUFLAGS += -m 256 -nographic -cpu SandyBridge,rdtscp -rtc base=utc

objs += kernel/arch/x64/boot.o

resea.iso: kernel.elf
	cp kernel.elf boot/x64/isofiles/kernel.elf
	$(PROGRESS) GRUB $@
	grub-mkrescue -o $@ boot/x64/isofiles

bochs: resea.iso
	$(BOCHS) -qf boot/x64/bochsrc
