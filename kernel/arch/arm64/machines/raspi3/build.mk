objs-$(CONFIG_MACHINE_RASPI3) += boot.o peripherals.o mp.o

CFLAGS += -mcpu=cortex-a53
QEMUFLAGS += -M raspi3 -smp 4

# Raspberry Pi's kernel8.img.
.PHONY: image
image: $(BUILD_DIR)/kernel8.img

$(BUILD_DIR)/kernel8.img: $(kernel_image)
	$(PROGRESS) "OBJCOPY" $@
	$(OBJCOPY) -Obinary $< $@
