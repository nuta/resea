# Build options.
TARGET := x64
ARCH := $(TARGET)
BUILD  ?= debug
BUILD_DIR := build
INIT := memmgr
LLVM_PREFIX := /usr/local/opt/llvm/bin/
GRUB_PREFIX := i386-elf-

# Set 1 to enable verbose output.
V =

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

# Variables internally used in the build system.
initfs_files :=

# Commands.
PYTHON3  ?= python3
XARGO    ?= xargo
CC       := $(LLVM_PREFIX)clang
LD       := $(LLVM_PREFIX)ld.lld
NM       := $(LLVM_PREFIX)llvm-nm
OBJCOPY  := $(LLVM_PREFIX)llvm-objcopy
PROGRESS := printf "  \\033[1;35m%8s  \\033[1;m%s\\033[m\\n"

XARGOFLAGS += --quiet
ifeq ($(BUILD), release)
XARGOFLAGS += --release
endif

RUSTFLAGS += -Z emit-stack-sizes
CFLAGS += --target=x86_64
CFLAGS += -mcmodel=large -mno-red-zone
CFLAGS += -mno-mmx -mno-sse -mno-sse2 -mno-avx -mno-avx2

# arch-specific build rules.
include kernel/arch/$(ARCH)/arch.mk

# Kernel build rules.
include kernel/kernel.mk

# Emulator options.
QEMUFLAGS += $(if $(GUI),,-nographic)

# Commands.
.PHONY: build
build: $(BUILD_DIR)/kernel.elf $(BUILD_DIR)/compile_commands.json

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

# IDL stub.
$(BUILD_DIR)/idl.json: misc/interfaces.idl tools/parse-idl-file.py
	$(PROGRESS) "PARSEIDL" $@
	$(PYTHON3) tools/parse-idl-file.py $< $@

libs/resea/idl/mod.rs: $(BUILD_DIR)/idl.json tools/genstub.py
	$(PROGRESS) "GENSTUB" libs/resea/idl
	$(PYTHON3) tools/genstub.py $< libs/resea/idl

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
		--cflags="$(CFLAGS)"                      \
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

# Initfs.
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
