objs-y += boot.o setup.o console.o task.o vm.o switch.o trap.o

ldflags-y += -T$(dir)/kernel.ld

QEMU := $(QEMU_PREFIX)qemu-system-riscv32
QEMUFLAGS += -machine virt -bios none -d unimp,guest_errors

# Append RISC-V specific C compiler flags globally (including userspace
# programs).
CFLAGS += --target=riscv32 -mcmodel=medany
