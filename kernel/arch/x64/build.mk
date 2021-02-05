objs-y += task.o vm.o serial.o boot.o init.o interrupt.o trap.o mp.o
objs-$(CONFIG_HYPERVISOR) += hv.o
objs-$(CONFIG_X64_PRINTK_IN_SCREEN) += screen.o

QEMU  ?= qemu-system-x86_64
BOCHS ?= bochs

CFLAGS += --target=x86_64 -mcmodel=large -fno-omit-frame-pointer
CFLAGS += -mno-red-zone -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2

QEMUFLAGS += -m 512 -cpu IvyBridge,rdtscp -rtc base=utc -serial mon:stdio
QEMUFLAGS += -no-reboot -boot d -device isa-debug-exit -d guest_errors,unimp
QEMUFLAGS += $(if $(SMP), -smp $(SMP))
QEMUFLAGS += $(if $(GUI),,-nographic)

ifneq ($(VMX),)
QEMUFLAGS += -enable-kvm -cpu host,vmx
endif

ifneq ($(HD_AUDIO),)
QEMUFLAGS += -device intel-hda,debug=3 -device hda-duplex,debug=3
endif

ifneq ($(E1000),)
QEMUFLAGS += -netdev user,id=net0,hostfwd=tcp:127.0.0.1:1234-:80
QEMUFLAGS += -object filter-dump,id=fiter0,netdev=net0,file=e1000.pcap
QEMUFLAGS += -device e1000,netdev=net0,mac=52:54:00:12:34:56
endif

ifneq ($(VIRTIO_NET),)
ifneq ($(VIRTIO_LEGACY),)
QEMUFLAGS += -device virtio-net,netdev=net1,disable-legacy=off,disable-modern=on
else
QEMUFLAGS += -device virtio-net,netdev=net1,packed=on
endif
QEMUFLAGS += -object filter-dump,id=fiter1,netdev=net1,file=virtio.pcap
QEMUFLAGS += -netdev user,id=net1,hostfwd=tcp:127.0.0.1:1234-:80
endif

ifneq ($(VIRTIO_GPU),)
QEMUFLAGS +=-vga none -device virtio-gpu,packed=on
endif

.PHONY: run
run: $(kernel_image)
	cp $(kernel_image) $(BUILD_DIR)/resea.qemu.elf
	./tools/make-bootable-on-qemu.py $(BUILD_DIR)/resea.qemu.elf
	$(PROGRESS) "RUN" $<
	$(QEMU) $(QEMUFLAGS) -kernel $(BUILD_DIR)/resea.qemu.elf

.PHONY: bochs
bochs: $(BUILD_DIR)/resea.iso
	$(PROGRESS) BOCHS $<
	$(BOCHS) -qf tools/bochsrc

.PHONY: iso
iso: $(BUILD_DIR)/resea.iso

$(BUILD_DIR)/resea.iso: $(kernel_image)
	mkdir -p $(BUILD_DIR)/isofiles/boot/grub
	cp tools/grub.cfg $(BUILD_DIR)/isofiles/boot/grub/grub.cfg
	cp $(kernel_image) $(BUILD_DIR)/isofiles/resea.elf
	$(PROGRESS) GRUB $@
	$(GRUB_PREFIX)grub-mkrescue --xorriso=$(PWD)/tools/xorriso-wrapper \
		-o $@ $(BUILD_DIR)/isofiles
