arch_dir := kernel/arch/$(ARCH)

# LLD options.
KERNEL_LDFLAGS += --script=$(arch_dir)/$(ARCH).ld

ifeq ($(BUILD), debug)
KERNEL_CFLAGS += -O1 -fsanitize=undefined -DDEBUG_BUILD
else
KERNEL_CFLAGS += -O3 -DRELEASE_BUILD
endif

# Clang options.
KERNEL_CFLAGS += -std=c11 -ffreestanding -fno-builtin -nostdlib
KERNEL_CFLAGS += -fno-omit-frame-pointer
KERNEL_CFLAGS += -g3 -fstack-size-section
KERNEL_CFLAGS += -Wall -Wextra
KERNEL_CFLAGS += -Werror=implicit-function-declaration
KERNEL_CFLAGS += -Werror=int-conversion
KERNEL_CFLAGS += -Werror=incompatible-pointer-types
KERNEL_CFLAGS += -Werror=shift-count-overflow
KERNEL_CFLAGS += -Werror=return-type
KERNEL_CFLAGS += -Werror=pointer-integer-compare
KERNEL_CFLAGS += -Werror=tautological-constant-out-of-range-compare
KERNEL_CFLAGS += -Ikernel/include -I$(arch_dir)/include
KERNEL_CFLAGS += -DKERNEL -DINITFS_BIN='"$(BUILD_DIR)/initfs.bin"'

kernel_objs +=  \
	$(BUILD_DIR)/kernel/boot.o \
	$(BUILD_DIR)/kernel/channel.o \
	$(BUILD_DIR)/kernel/process.o \
	$(BUILD_DIR)/kernel/memory.o \
	$(BUILD_DIR)/kernel/thread.o \
	$(BUILD_DIR)/kernel/server.o \
	$(BUILD_DIR)/kernel/ipc.o \
	$(BUILD_DIR)/kernel/timer.o \
	$(BUILD_DIR)/kernel/initfs.o \
	$(BUILD_DIR)/kernel/support/printk.o \
	$(BUILD_DIR)/kernel/support/string.o \
	$(BUILD_DIR)/kernel/support/debug.o

$(BUILD_DIR)/kernel.elf: $(kernel_objs) $(arch_dir)/$(ARCH).ld tools/link.py Makefile
	$(PROGRESS) "LINK" $@
	$(PYTHON3) tools/link.py                      \
		--cc="$(CC)"                          \
		--cflags="$(KERNEL_CFLAGS)"           \
		--ld="$(LD)"                          \
		--ldflags="$(KERNEL_LDFLAGS)"         \
		--nm="$(NM)"                          \
		--objcopy="$(OBJCOPY)"                \
		--build-dir="$(BUILD_DIR)"            \
		--mapfile="$(BUILD_DIR)/kernel.map"   \
		--outfile="$@"                        \
		$(kernel_objs)
	$(MAKE) $(BUILD_DIR)/compile_commands.json

$(BUILD_DIR)/kernel/initfs.o: $(BUILD_DIR)/initfs.bin

# C/assembly source file build rules.
$(BUILD_DIR)/%.o: %.c
	$(PROGRESS) "CC" $@
	mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json) -c -o $@ $<

$(BUILD_DIR)/%.o: %.S
	$(PROGRESS) "CC" $@
	mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json) -c -o $@ $<

# The JSON compilation database.
# https://clang.llvm.org/docs/JSONCompilationDatabase.html
$(BUILD_DIR)/compile_commands.json: $(compiled_objs)
	$(PROGRESS) "GEN" $(BUILD_DIR)/compile_commands.json
	$(PYTHON3) tools/merge-compile-commands.py \
		-o $(BUILD_DIR)/compile_commands.json \
		$(shell find $(BUILD_DIR) -type f -name "*.json" \
			| grep -v compile_commands.json)

-include $(BUILD_DIR)/kernel/*.deps
