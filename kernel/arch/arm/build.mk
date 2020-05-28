obj-y += boot.o init.o vm.o mp.o task.o switch.o peripherals.o interrupt.o

QEMU  ?= qemu-system-arm

CFLAGS += --target=armv6m-none-eabi -mcpu=cortex-m0
LDFLAGS += $(shell arm-none-eabi-gcc -mcpu=cortex-m0 -print-libgcc-file-name)

QEMUFLAGS += -M microbit -serial mon:stdio
QEMUFLAGS += $(if $(GUI),,-nographic)
QEMUFLAGS += $(if $(GDB),-S -s,)

.PHONY: run
run: $(BUILD_DIR)/resea.hex
	$(QEMU) $(QEMUFLAGS) -device loader,file=$<

.PHONY: hex
hex: $(BUILD_DIR)/resea.hex

$(BUILD_DIR)/resea.hex: $(kernel_image)
	$(OBJCOPY) -Oihex $< $@
