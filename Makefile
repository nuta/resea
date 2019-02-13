default: build

V ?=
ARCH ?= x64
BUILD ?= debug
IDLS = kernel memmgr pager putchar
STARTUPS ?= benchmark

# Variables.
repo_dir := $(shell pwd)
arch_dir := kernel/arch/$(ARCH)
idl_files := $(foreach name, $(IDLS), libs/resea/src/idl/$(name).rs)
initfs_files = $(foreach name, $(STARTUPS), initfs/startups/$(name).elf)

# Functions.
build_dir = target/$(strip $(1))/$(ARCH)/$(BUILD)

# Use Homebrew (`llvm' package) version to generate --target=x86_64 objects
# on macOS.
ifeq ($(shell uname), Darwin)
LLVM_PREFIX ?= /usr/local/opt/llvm/bin/
BINUTILS_PREFIX = /usr/local/opt/binutils/bin/g
endif

# Build utilities.
CC := $(LLVM_PREFIX)clang
LD := $(LLVM_PREFIX)ld.lld
PYTHON3 ?= python3
NM ?= $(BINUTILS_PREFIX)nm
OBJCOPY ?= $(BINUTILS_PREFIX)objcopy
PROGRESS ?= printf "  \033[1;35m%8s  \033[1;m%s\033[m\n"

# Load flags specified in the command line.
CFLAGS := $(CFLAGS)
LDFLAGS := $(LDFLAGS)

# Common flags.
LDFLAGS += --gc-sections
CFLAGS += -O2 -g3 -std=c11 -ffreestanding -fno-builtin -nostdlib
CFLAGS += -Wall -Wextra
CFLAGS += -Werror=implicit-function-declaration
CFLAGS += -Werror=int-conversion
CFLAGS += -Werror=incompatible-pointer-types
CFLAGS += -Werror=shift-count-overflow
CFLAGS += -Werror=shadow
XARGOFLAGS += --quiet
RUSTFLAGS += -C soft-float --emit=link,dep-info -Z external-macro-backtrace

ifeq ($(BUILD), release)
XARGOFLAGS += --release
endif

# Files and directories to be deleted by `make clean`.
clean = target initfs kernel.elf kernel.map kernel.symbols memmgr.bin initfs.bin $(kernel_objs)

# Arch-dependent variables.
kernel_objs := $(call build_dir, kernel)/libkernel.a
include $(arch_dir)/arch.mk

# Disable builtin implicit rules.
MAKEFLAGS += --no-builtin-rules
.SUFFIXES:

# Enable verbose output if $(V) is not empty.
$(V).SILENT:

# Commands.
.PHONY: build clean
build: kernel.elf

clean:
	rm -rf $(clean)

# Kernel build rules.
kernel.elf: $(kernel_objs) $(arch_dir)/$(ARCH).ld
	$(PROGRESS) "LD" kernel.nosymbols.elf
	$(LD) $(LDFLAGS) --Map=kernel.map --script=$(arch_dir)/$(ARCH).ld -o kernel.nosymbols.elf $(kernel_objs)
	$(PROGRESS) "SYMBOLS" $@
	$(PYTHON3) ./tools/embed-symbols.py --nm $(NM) kernel.nosymbols.elf
	mv kernel.nosymbols.elf $@
	$(PROGRESS) "GEN" kernel.symbols
	$(BINUTILS_PREFIX)nm -g $@ | awk '{ print $$1, $$3 }' > kernel.symbols

$(call build_dir, kernel)/libkernel.a: kernel/Cargo.toml initfs.bin Makefile
	$(PROGRESS) "XARGO" kernel
	cd kernel && \
		CARGO_TARGET_DIR="$(repo_dir)/target/kernel" \
		RUST_TARGET_PATH="$(repo_dir)/$(arch_dir)" \
		RUSTFLAGS="$(RUSTFLAGS)" xargo build $(XARGOFLAGS) --target $(ARCH)
	# Make dependency paths relative.
	sed -i "" "s#$(repo_dir)/##g" $(call build_dir, kernel)/libkernel.d

-include $(call build_dir, kernel)/libkernel.d

# Initfs build rules.
initfs.bin: memmgr.bin $(initfs_files)
	$(PROGRESS) "MKINITFS" $@
	$(PYTHON3) ./tools/mkinitfs.py -s memmgr.bin -o $@ initfs

memmgr.bin: target/memmgr/memmgr_$(ARCH)/$(BUILD)/memmgr
	$(PROGRESS) "OBJCOPY" $@
	$(OBJCOPY) -j.startup -j.text -j.data -j.rodata -j.bss -Obinary $< $@

# Userland executable build rules.
-include $(foreach app, memmgr $(STARTUPS), target/mk/$(app).mk)

target/mk/memmgr.mk: tools/gen-user-makefile.py Makefile
	$(PROGRESS) "GEN" $@
	mkdir -p target/mk
	$(PYTHON3) ./tools/gen-user-makefile.py \
		--target-path servers/memmgr/src/arch/$(ARCH) \
		--target memmgr_$(ARCH) \
		--path servers/memmgr \
		--arch $(ARCH) \
		--output $@ \
		--name memmgr

target/mk/%.mk: tools/gen-user-makefile.py Makefile
	$(PROGRESS) "GEN" $@
	mkdir -p target/mk
	$(PYTHON3) ./tools/gen-user-makefile.py \
		--target-path libs/resea/src/arch/$(ARCH) \
		--target user_$(ARCH) \
		--path apps/$(basename $(@F)) \
		--arch $(ARCH) \
		--output $@ \
		--add-startup \
		--name $(basename $(@F))

# IDL build rules.
$(idl_files): libs/resea/src/idl/%.rs: idl/%.idl Makefile tools/idl.py
	$(PROGRESS) "IDL" $@
	mkdir -p libs/resea/src/idl
	$(PYTHON3) ./tools/idl.py -o $@ $<

libs/resea/src/idl/mod.rs: $(idl_files)
	$(PROGRESS) "GEN" $@
	for idl in $(IDLS); do echo "pub mod $$idl;"; done > $@

# C/asm source file build rules.
%.o: %.c Makefile
	$(PROGRESS) "CC" $@
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.S Makefile
	$(PROGRESS) "CC" $@
	$(CC) $(CFLAGS) -c -o $@ $<
