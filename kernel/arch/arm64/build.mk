obj-y += boot.o trap.o init.o vm.o mp.o task.o switch.o peripherals.o interrupt.o
obj-y += usercopy.o

QEMU  ?= qemu-system-aarch64

CFLAGS += --target=aarch64-none-eabi -mcpu=cortex-a53 -mcmodel=large
CFLAGS += -mgeneral-regs-only
LDFLAGS +=

QEMUFLAGS += -M raspi3 -serial mon:stdio -semihosting -d guest_errors,unimp
QEMUFLAGS += $(if $(GUI),,-nographic)
QEMUFLAGS += $(if $(GDB),-S -s,)

.PHONY: run
run: $(BUILD_DIR)/kernel8.img
	$(PROGRESS) "RUN" $<
	$(QEMU) $(QEMUFLAGS) -kernel $<

# Raspberry Pi's kernel8.img.
.PHONY: image
image: $(BUILD_DIR)/kernel8.img

$(BUILD_DIR)/kernel8.img: $(kernel_image)
	$(PROGRESS) "OBJCOPY" $@
	$(OBJCOPY) -Obinary $< $@
