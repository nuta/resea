obj-y += boot.o init.o vm.o mp.o task.o peripherals.o interrupt.o

QEMU  ?= qemu-system-

CFLAGS += --target=x86_64 -mcmodel=large
LDFLAGS +=
QEMUFLAGS +=
# QEMUFLAGS += $(if $(GUI),,-nographic)
# QEMUFLAGS += $(if $(GDB),-S -s,)

.PHONY: run
run: $(kernel_image)
	$(PROGRESS) "RUN" $(kernel_image)
	echo "*"
	echo "*  Implement 'make run' in kernel/<ARCH>/build.mk!"
	echo "*"
#	$(QEMU) $(QEMUFLAGS) $<
