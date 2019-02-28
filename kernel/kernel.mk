KERNEL_CFLAGS += -Ikernel/include -Ikernel/arch/$(ARCH)/include
kernel_objs += \
	kernel/boot.o \
	kernel/process.o \
	kernel/memory.o \
	kernel/thread.o \
	kernel/ipc.o \
	kernel/server.o \
	kernel/timer.o \
	kernel/printk.o \
	kernel/collections.o \
	kernel/debug.o

# C/asm source file build rules.
kernel/%.o: kernel/%.c Makefile
	$(PROGRESS) "CC(K)" $@
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) -c -o $@ $<

kernel/%.o: kernel/%.S Makefile
	$(PROGRESS) "CC(K)" $@
	$(CC) $(CFLAGS) $(KERNEL_CFLAGS) -c -o $@ $<
