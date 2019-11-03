# Specify "linux" toolchain to use the KASan.
KERNEL_CFLAGS += --target=x86_64-pc-linux-elf
KERNEL_CFLAGS += -mcmodel=large -mno-red-zone
KERNEL_CFLAGS += -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2

HDD_DIR = $(BUILD_DIR)/hdd
BOCHS ?= bochs
QEMU ?= qemu-system-x86_64
QEMUFLAGS += -smp 2 -m 512 -cpu IvyBridge,rdtscp -rtc base=utc -serial mon:stdio
QEMUFLAGS += -no-reboot -device isa-debug-exit,iobase=0xf4,iosize=0x04
QEMUFLAGS += -boot d

kernel_objs += \
	$(BUILD_DIR)/kernel/arch/x64/boot.o \
	$(BUILD_DIR)/kernel/arch/x64/apic.o \
	$(BUILD_DIR)/kernel/arch/x64/asan.o \
	$(BUILD_DIR)/kernel/arch/x64/handler.o \
	$(BUILD_DIR)/kernel/arch/x64/interrupt.o \
	$(BUILD_DIR)/kernel/arch/x64/paging.o \
	$(BUILD_DIR)/kernel/arch/x64/screen.o \
	$(BUILD_DIR)/kernel/arch/x64/serial.o \
	$(BUILD_DIR)/kernel/arch/x64/setup.o \
	$(BUILD_DIR)/kernel/arch/x64/mp.o \
	$(BUILD_DIR)/kernel/arch/x64/thread.o

.PHONY: bochs run iso
bochs: $(BUILD_DIR)/resea.iso
	$(PROGRESS) GEN $(BUILD_DIR)/kernel.symbols
	$(NM) $(BUILD_DIR)/kernel.elf | awk '{ print $$1, $$3 }' \
		> $(BUILD_DIR)/kernel.symbols
	$(PROGRESS) BOCHS $<
	$(BOCHS) -qf misc/bochsrc

run: $(BUILD_DIR)/resea.iso
	$(PROGRESS) QEMU $(BUILD_DIR)/kernel.iso
	$(QEMU) $(QEMUFLAGS) -cdrom $<

iso: $(BUILD_DIR)/resea.iso

$(BUILD_DIR)/resea.iso: build
	mkdir -p $(BUILD_DIR)/isofiles/boot/grub
	cp misc/grub.cfg $(BUILD_DIR)/isofiles/boot/grub/grub.cfg
	cp $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/isofiles/kernel.elf
	$(PROGRESS) GRUB $@
	$(GRUB_PREFIX)grub-mkrescue --xorriso=$(PWD)/misc/xorriso-wrapper \
		-o $@ $(BUILD_DIR)/isofiles
