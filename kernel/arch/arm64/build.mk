obj-y += boot.o trap.o init.o vm.o mp.o task.o switch.o peripherals.o interrupt.o
obj-y += usercopy.o

QEMU  ?= qemu-system-aarch64

CFLAGS += --target=aarch64-none-eabi -mcpu=cortex-a53 -mcmodel=large
CFLAGS += -mgeneral-regs-only
LDFLAGS +=

QEMUFLAGS += -M raspi3 -serial mon:stdio -semihosting
QEMUFLAGS += $(if $(GUI),,-nographic)
QEMUFLAGS += $(if $(GDB),-S -s,)

.PHONY: run
run: $(kernel_image)
	$(QEMU) $(QEMUFLAGS) -kernel $<
