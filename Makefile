V =
ARCH ?= x64
BUILD ?= debug
IDLS = kernel memmgr pager putchar
RUSTFLAGS += -C soft-float --emit=link,dep-info -Z external-macro-backtrace
REPO_DIR := $(shell pwd)
BUILD_DIR := $(REPO_DIR)/target
ARCH_DIR := $(REPO_DIR)/kernel/arch/$(ARCH)
KERNEL_A := $(BUILD_DIR)/kernel/$(ARCH)/$(BUILD)/libresea_kernel.a
objs := $(KERNEL_A)

default: build
$(V).SILENT:
.SECONDARY:
.SUFFIXES:

ifeq ($(shell uname), Darwin)
# Use Homebrew (`llvm' package) version to emit --target=x86_64 objects.
LLVM_PREFIX ?= /usr/local/opt/llvm/bin/
BINUTILS_PREFIX = /usr/local/opt/binutils/bin/g
endif

CC = $(LLVM_PREFIX)clang
LD = $(LLVM_PREFIX)ld.lld
NM = $(BINUTILS_PREFIX)nm
OBJCOPY = $(BINUTILS_PREFIX)objcopy
PROGRESS ?= printf "  \033[1;35m%8s  \033[1;m%s\033[m\n"

override LDFLAGS := $(LDFLAGS) --gc-sections --Map=kernel.map
override CFLAGS := $(CFLAGS) \
	-std=c17 \
    -Wall \
	-Werror=implicit-function-declaration \
	-Werror=int-conversion \
	-Werror=incompatible-pointer-types \
	-Werror=shift-count-overflow \
	-Werror=shadow

XARGOFLAGS += --quiet
ifeq ($(BUILD), release)
XARGOFLAGS += --release
endif

include kernel/arch/$(ARCH)/arch.mk

.PHONY: build run test clean setup book
build: kernel.elf

setup:
	./tools/setup

kernel.qemu.elf: kernel.elf
	$(PROGRESS) TWEAK $@
	cp kernel.elf $@
	./tools/tweak-elf-for-qemu.py $@
	$(PROGRESS) QEMU $@

run: kernel.qemu.elf
	$(PROGRESS) QEMU kernel.qemu.elf
	./tools/run-qemu.py -- $(QEMU) $(QEMUFLAGS) -kernel kernel.qemu.elf

test:
	cargo test

book:
	./tools/gen-book.py docs kernel

clean:
	cd kernel && cargo clean
	cd servers/memmgr && cargo clean
	rm -rf target kernel.elf arch/x64/isofiles/kernel.elf resea.iso kernel.map kernel.qemu.elf kernel.nosymbols.elf $(objs) serial.log resea.iso memmgr.bin initfs.bin initfs

kernel.elf: $(objs)
	$(PROGRESS) "LD" kernel.nosymbols.elf
	$(LD) $(LDFLAGS) --script=$(ARCH_DIR)/$(ARCH).ld -o kernel.nosymbols.elf $(objs)
	$(PROGRESS) "SYMBOLS" $@
	cp kernel.nosymbols.elf kernel.nosymbols.elf.tmp
	./tools/embed-symbols.py --nm $(NM) kernel.nosymbols.elf.tmp
	mv kernel.nosymbols.elf.tmp $@
	rm kernel.nosymbols.elf
	$(PROGRESS) "GEN" kernel.symbols
	$(BINUTILS_PREFIX)nm -g $@ | awk '{ print $$1, $$3 }' > kernel.symbols

$(KERNEL_A): kernel/Cargo.toml initfs.bin Makefile
	$(PROGRESS) "XARGO" kernel
	cd kernel && \
		CARGO_TARGET_DIR="$(BUILD_DIR)/kernel" \
		RUST_TARGET_PATH="$(ARCH_DIR)" \
		RUSTFLAGS="$(RUSTFLAGS)" xargo build $(XARGOFLAGS) --target $(ARCH)

initfs.bin: memmgr.bin initfs/startups/benchmark.elf
	$(PROGRESS) "MKINITFS" initfs.bin
	./tools/mkinitfs.py -s memmgr.bin -o $@ initfs

%.o: %.S Makefile
	$(PROGRESS) "CC" $@
	$(CC) $(CFLAGS) -c -o $@ $<

memmgr.bin: $(BUILD_DIR)/memmgr/memmgr_$(ARCH)/$(BUILD)/memmgr
	$(PROGRESS) "OBJCOPY" memmgr.bin
	$(OBJCOPY) -j.startup -j.text -j.data -j.rodata -j.bss -Obinary $< $@

idl_files := $(foreach name, $(IDLS), libs/resea/src/idl/$(name).rs)
$(idl_files): libs/resea/src/idl/%.rs: idl/%.idl Makefile tools/idl.py
	$(PROGRESS) "IDL" $@
	./tools/idl.py -o $@ $<

libs/resea/src/idl/mod.rs: $(idl_files)
	$(PROGRESS) "GEN" $@
	for idl in $(IDLS); do echo "pub mod $$idl;"; done > $@

$(BUILD_DIR)/memmgr/memmgr_$(ARCH)/$(BUILD)/memmgr: servers/memmgr/src/arch/x64/start.o servers/memmgr/src/arch/x64/memmgr.ld Makefile libs/resea/src/idl/mod.rs
	$(PROGRESS) "XARGO" servers/memmgr
	cd servers/memmgr && \
		CARGO_TARGET_DIR="$(BUILD_DIR)/memmgr" \
		RUST_TARGET_PATH="$(REPO_DIR)/servers/memmgr/src/arch/$(ARCH)" \
		RUSTFLAGS="$(RUSTFLAGS)" \
			xargo build $(XARGOFLAGS) --target memmgr_$(ARCH)

$(BUILD_DIR)/benchmark/user_$(ARCH)/$(BUILD)/benchmark: libs/resea/src/arch/x64/start.o libs/resea/src/arch/x64/user.ld Makefile libs/resea/src/idl/mod.rs
	$(PROGRESS) "XARGO" apps/benchmark
	cd apps/benchmark && \
		CARGO_TARGET_DIR="$(BUILD_DIR)/benchmark" \
		RUST_TARGET_PATH="$(REPO_DIR)/libs/resea/src/arch/$(ARCH)" \
		RUSTFLAGS="$(RUSTFLAGS)" xargo build $(XARGOFLAGS) --target user_$(ARCH)
	$(BINUTILS_PREFIX)strip $@

initfs/startups/benchmark.elf: $(BUILD_DIR)/benchmark/user_$(ARCH)/$(BUILD)/benchmark
	mkdir -p initfs/startups
	cp $< $@

-include $(BUILD_DIR)/kernel/$(ARCH)/$(BUILD)/libresea_kernel.d
-include $(BUILD_DIR)/memmgr/memmgr_$(ARCH)/$(BUILD)/memmgr.d
-include $(BUILD_DIR)/benchmark/user_$(ARCH)/$(BUILD)/memmgr.d
