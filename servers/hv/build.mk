name := hv
description := [Work-In-Progress] A hypervisor (similar to QEMU/KVM).
objs-y := main.o guest.o mm.o ioport.o pci.o virtio_blk.o x64.o kernel_elf.o
libs-y := virtio
libs-y := driver # FIXME: virtio's dependency; we don't need it

HV_GUEST_IMAGE ?= vmlinux.elf
$(BUILD_DIR)/servers/hv/kernel_elf.o: $(HV_GUEST_IMAGE)
	mkdir -p $(dir $(@:.o=.S))
	echo ".rodata; .align 8; .global __kernel_elf, __kernel_elf_end; __kernel_elf: .incbin \"$(HV_GUEST_IMAGE)\"; __kernel_elf_end:" > $(@:.o=.S)
	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.S)

# sample := longmode
# $(BUILD_DIR)/servers/hv/kernel_elf.o:
# 	$(MAKE) -C servers/hv/samples/$(sample)
# 	mkdir -p $(dir $(@:.o=.S))
# 	echo ".rodata; .align 8; .global __kernel_elf, __kernel_elf_end; __kernel_elf: .incbin \"servers/hv/samples/$(sample)/kernel.elf\"; __kernel_elf_end:" > $(@:.o=.S)
# 	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.S)
