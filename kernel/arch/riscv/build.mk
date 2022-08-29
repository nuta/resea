objs-y += boot.o setup.o console.o vm.o

ldflags-y += -T$(dir)/kernel.ld

QEMU := $(QEMU_PREFIX)qemu-system-riscv32
QEMUFLAGS += -machine virt -bios none -d guest_errors,unimp

# Append RISC-V specific C compiler flags globally (including userspace
# programs).
CFLAGS += --target=riscv32
