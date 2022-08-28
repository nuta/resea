objs-y += boot.o setup.o

ldflags-y += -T$(dir)/kernel.ld

# Append RISC-V specific C compiler flags globally (including userspace
# programs).
CFLAGS += --target=riscv32
