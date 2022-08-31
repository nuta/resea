objs-y += boot.o setup.o uart.o task.o vm.o switch.o trap.o

ldflags-y += -T$(dir)/kernel.ld

QEMU := $(QEMU_PREFIX)qemu-system-riscv32
# QEMUFLAGS += -machine virt -bios none -d unimp,guest_errors
QEMUFLAGS += -m 128 -machine virt -bios none

# Append RISC-V specific C compiler flags globally (including userspace
# programs).
CFLAGS += --target=riscv32
