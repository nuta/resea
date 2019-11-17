#
#  Build options
#
V ?=
TARGET := x64
ARCH := $(TARGET)
BUILD  ?= debug
BUILD_DIR := build
INIT := memmgr
STARTUPS := fatfs net pcat ide e1000
LLVM_PREFIX ?= /usr/local/opt/llvm/bin/
GRUB_PREFIX ?= i386-elf-

PYTHON3  ?= python3
XARGO    ?= xargo
CC       := $(LLVM_PREFIX)clang
LD       := $(LLVM_PREFIX)ld.lld
NM       := $(LLVM_PREFIX)llvm-nm
OBJCOPY  := $(LLVM_PREFIX)llvm-objcopy
PROGRESS := printf "  \\033[1;35m%8s  \\033[1;m%s\\033[m\\n"

ifeq ($(BUILD), debug)
COMMON_CFLAGS += -O1 -fsanitize=undefined -DDEBUG_BUILD
else
COMMON_CFLAGS += -O3 -DRELEASE_BUILD
XARGOFLAGS += --release
endif

# Clang options (kernel).
KERNEL_CFLAGS += $(COMMON_CFLAGS)
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

# LLD options (kernel).
KERNEL_LDFLAGS += --script=$(arch_dir)/$(ARCH).ld

# Cargo/rustc options (userland).
XARGOFLAGS += --quiet
RUSTFLAGS += -Z emit-stack-sizes -Z external-macro-backtrace

# Clang options (userland: use only for compiling the symbol table)
USER_CFLAGS += $(COMMON_CFLAGS)

# Your own local build configuration.
-include .build.mk

# ------------------------------------------------------------------------------

# The default build target.
.PHONY: default
default: build

# Enable verbose output if $(V) is set.
ifeq ($(V),)
.SILENT:
endif

# Disable builtin implicit rules and variables.
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

initfs_files :=

#
#  Build Targets
#
.PHONY: build
build: $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/compile_commands.json

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

#
#  Kernel
#
arch_dir := kernel/arch/$(ARCH)
include $(arch_dir)/arch.mk

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
	$(BUILD_DIR)/kernel/support/stack_protector.o \
	$(BUILD_DIR)/kernel/support/backtrace.o \
	$(BUILD_DIR)/kernel/support/kdebug.o \
	$(BUILD_DIR)/kernel/support/kasan.o \
	$(BUILD_DIR)/kernel/support/ubsan.o

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
$(BUILD_DIR)/%.o: %.c kernel/include/idl.h
	$(PROGRESS) "CC" $@
	mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json) -c -o $@ $<

$(BUILD_DIR)/%.o: %.S
	$(PROGRESS) "CC" $@
	mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json) -c -o $@ $<

# The JSON compilation database.
# https://clang.llvm.org/docs/JSONCompilationDatabase.html
$(BUILD_DIR)/compile_commands.json: $(kernel_objs)
	$(PROGRESS) "GEN" $(BUILD_DIR)/compile_commands.json
	$(PYTHON3) tools/merge-compile-commands.py \
		-o $(BUILD_DIR)/compile_commands.json \
		$(shell find $(BUILD_DIR)/kernel -type f -name "*.json")

-include $(BUILD_DIR)/kernel/*.deps

#
#  Userland
#

# Server executables.
$(BUILD_DIR)/servers/%.elf: libs/resea/idl/mod.rs tools/link.py Makefile
	$(PROGRESS) "XARGO" servers/$(name)
	cd servers/$(name) && \
		PROGRAM_NAME="$(name)" \
		RUST_TARGET_PATH="$(PWD)/libs/resea/arch/$(TARGET)"  \
		RUSTFLAGS="$(RUSTFLAGS)"                             \
		$(XARGO) build --target user_$(TARGET)               \
			--target-dir ../../$(BUILD_DIR)/servers/$(name) \
			$(XARGOFLAGS)
	$(PYTHON3) ./tools/fix-deps-file.py \
		$(BUILD_DIR)/servers/$(name)/user_$(TARGET)/$(BUILD)/lib$(name).d \
		$(BUILD_DIR)/servers/$(name).elf
	$(PROGRESS) "LINK" $@
	$(PYTHON3) tools/link.py                          \
		--cc="$(CC)"                              \
		--cflags="$(USER_CFLAGS)"                 \
		--ld="$(LD)"                              \
		--ldflags="$(ldflags)"                    \
		--nm="$(NM)"                              \
		--objcopy="$(OBJCOPY)"                    \
		--build-dir="$(BUILD_DIR)/servers/$(name)"   \
		--mapfile="$(BUILD_DIR)/servers/$(name).map" \
		--outfile="$(BUILD_DIR)/servers/$(name).elf" \
		$(BUILD_DIR)/servers/$(name)/user_$(TARGET)/$(BUILD)/lib$(name).a
	$(NM) $@ | awk '{ print $$1, $$3 }' > $(@:.elf=.symbols)

define server-make-rule
$(BUILD_DIR)/servers/$(1).elf: name := $(1)
$(BUILD_DIR)/servers/$(1).elf: ldflags := \
	$(if $(wildcard servers/$(1)/$(1)_$(TARGET).ld), \
		--script=servers/$(1)/$(1)_$(TARGET).ld, \
		--script=libs/resea/arch/$(TARGET)/user_$(TARGET).ld)
-include $(BUILD_DIR)/servers/$(1)/user_$(TARGET)/$(BUILD)/lib$(1).d
endef
$(foreach server, $(INIT) $(STARTUPS), $(eval $(call server-make-rule,$(server))))
initfs_files += $(foreach server, $(STARTUPS), $(BUILD_DIR)/initfs/startups/$(server).elf)

#
#  IPC stubs.
#
$(BUILD_DIR)/idl.json: interfaces.idl tools/parse-idl-file.py
	$(PROGRESS) "PARSEIDL" $@
	$(PYTHON3) tools/parse-idl-file.py $< $@

libs/resea/idl/mod.rs: $(BUILD_DIR)/idl.json tools/genstub.py
	$(PROGRESS) "GENSTUB" libs/resea/idl
	$(PYTHON3) tools/genstub.py $< libs/resea/idl

kernel/include/idl.h: $(BUILD_DIR)/idl.json tools/kgenstub.py
	$(PROGRESS) "KGENSTUB" $@
	$(PYTHON3) tools/kgenstub.py $< $@

#
# initfs
#
$(BUILD_DIR)/initfs.bin: $(initfs_files) $(BUILD_DIR)/$(INIT).bin tools/mkinitfs.py Makefile
	$(PROGRESS) "MKINITFS" $@
	$(PYTHON3) tools/mkinitfs.py          \
		-o $(@:.o=.bin)               \
		-s $(BUILD_DIR)/$(INIT).bin   \
		--file-list "$(initfs_files)" \
		$(BUILD_DIR)/initfs

$(BUILD_DIR)/initfs/startups/%.elf: $(BUILD_DIR)/servers/%.elf
	mkdir -p $(@D)
	cp $< $@

$(BUILD_DIR)/initfs/servers/%.elf: $(BUILD_DIR)/servers/%.elf
	mkdir -p $(@D)
	cp $< $@

$(BUILD_DIR)/initfs/apps/%.elf: $(BUILD_DIR)/apps/%.elf
	mkdir -p $(@D)
	cp $< $@

# Startup.
$(BUILD_DIR)/$(INIT).bin: $(BUILD_DIR)/servers/$(INIT).elf Makefile
	$(OBJCOPY) -j.initfs -j.text -j.data -j.rodata -j.bss -Obinary $< $@
