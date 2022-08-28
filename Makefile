V ?=
ARCH ?= riscv
BUILD_DIR ?= build

ifeq ($(shell uname), Darwin)
LLVM_PREFIX ?= /opt/homebrew/opt/llvm/bin/
endif

# Disable builtin implicit rules and variables.
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

# Enable verbose output if $(V) is set.
ifeq ($(V),)
.SILENT:
endif

top_dir := $(shell pwd)
kernel_elf := $(BUILD_DIR)/resea.elf

#
#  Commands
#
CC       := $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
LD       := $(LLVM_PREFIX)ld.lld$(LLVM_SUFFIX)
PROGRESS ?= printf "  \\033[1;96m%8s\\033[0m  \\033[1;m%s\\033[0m\\n"

CFLAGS += -g3 -std=c11 -ffreestanding -fno-builtin -nostdlib -nostdinc
CFLAGS += -Wall -Wextra
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -Werror=int-conversion
CFLAGS += -Werror=incompatible-pointer-types
CFLAGS += -Werror=shift-count-overflow
CFLAGS += -Werror=switch
CFLAGS += -Werror=return-type
CFLAGS += -Werror=pointer-integer-compare
CFLAGS += -Werror=tautological-constant-out-of-range-compare
CFLAGS += -Wno-unused-parameter
CFLAGS += -I$(top_dir)

QEMUFLAGS += -serial mon:stdio
QEMUFLAGS += $(if $(GUI),,-nographic)
QEMUFLAGS += $(if $(GDB),-S -s,)

executable := $(kernel_elf)
dir := kernel
include kernel/build.mk

.PHONY: build
build: $(kernel_elf)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: run
run: $(kernel_elf)
	$(QEMU) $(QEMUFLAGS) -kernel $(kernel_elf)

$(BUILD_DIR)/%.o: %.c Makefile
	$(PROGRESS) CC $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

$(BUILD_DIR)/%.o: %.S Makefile
	$(PROGRESS) CC $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)
