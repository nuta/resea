VERSION := v0.1.0

# Enable verbose output if $(V) is set.
ifeq ($(V),)
.SILENT:
endif

# The default build target.
.PHONY: default
default: build

-include .config.mk

# Determine if we need to load ".config".
non_config_targets := defconfig menuconfig
load_config := y
ifeq ($(filter-out $(non_config_targets), $(MAKECMDGOALS)),)
load_config :=
endif
# The default target (build) needs ".config".
ifeq ($(MAKECMDGOALS),)
load_config := y
endif

# Include other makefiles.
ifeq ($(load_config), y)
ifeq ($(wildcard .config.mk),)
$(error .config.mk does not exist (run 'make menuconfig' first))
endif

kernel_image := $(BUILD_DIR)/resea.elf
libs := resea

include kernel/arch/$(ARCH)/arch.mk
include libs/common/lib.mk
endif

CC         := $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
LD         := $(LLVM_PREFIX)ld.lld$(LLVM_SUFFIX)
NM         := $(LLVM_PREFIX)llvm-nm$(LLVM_SUFFIX)
OBJCOPY    := $(LLVM_PREFIX)llvm-objcopy$(LLVM_SUFFIX)
CLANG_TIDY := $(LLVM_PREFIX)clang-tidy$(LLVM_SUFFIX)
PROGRESS   := printf "  \\033[1;96m%8s\\033[0m  \\033[1;m%s\\033[0m\\n"
PYTHON3    ?= python3

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
CFLAGS += -fstack-size-section
CFLAGS += -Ilibs/common/include -Ilibs/common/arch/$(ARCH)
CFLAGS += -DVERSION='"$(VERSION)"'

ifeq ($(BUILD),release)
CFLAGS += -O2 -flto
else
CFLAGS += -O1 -DDEBUG -fsanitize=undefined
endif

# Disable builtin implicit rules and variables.
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

kernel_objs += main.o task.o ipc.o syscall.o memory.o printk.o kdebug.o

initfs_files := $(foreach name, $(SERVERS), $(BUILD_DIR)/user/$(name).elf)
kernel_objs := \
	$(addprefix $(BUILD_DIR)/kernel/kernel/, $(kernel_objs)) \
	$(addprefix $(BUILD_DIR)/kernel/libs/common/, $(libcommon_objs))
lib_objs := \
	$(foreach lib, $(libs), $(BUILD_DIR)/user/libs/$(lib).o) \
	$(addprefix $(BUILD_DIR)/user/libs/common/, $(libcommon_objs))

#
#  Build Commands
#
.PHONY: build
build: $(kernel_image) $(BUILD_DIR)/compile_commands.json

.PHONY: lint
lint:$(BUILD_DIR)/compile_commands.json
	find kernel servers libs -name "*.c" | xargs $(CLANG_TIDY) -p $(BUILD_DIR)

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: defconfig
defconfig: .config.mk

.PHONY: menuconfig
menuconfig:
	./tools/config.py --menuconfig

#
#  Build Rules
#
$(kernel_image): $(kernel_objs) $(BUILD_DIR)/kernel/__program_name.o \
		kernel/arch/$(ARCH)/kernel.ld \
		tools/nm2symbols.py tools/embed-symbols.py Makefile
	$(PROGRESS) "LD" $@
	$(LD) $(LDFLAGS) --script=kernel/arch/$(ARCH)/kernel.ld \
		-Map $(@:.elf=.map) -o $@.tmp \
		$(kernel_objs) $(BUILD_DIR)/kernel/__program_name.o
	$(PROGRESS) "GEN" $(@:.elf=.symbols)
	$(NM) $@.tmp | ./tools/nm2symbols.py > $(@:.elf=.symbols)
	$(PROGRESS) "SYMBOLS" $@
	./tools/embed-symbols.py $(@:.elf=.symbols) $@.tmp
	cp $@.tmp $@

$(BUILD_DIR)/initfs.bin: $(initfs_files) $(BUILD_DIR)/$(INIT).bin tools/mkinitfs.py
	$(PROGRESS) "MKINITFS" $@
	$(PYTHON3) tools/mkinitfs.py -o $@ --init $(BUILD_DIR)/$(INIT).bin \
		$(initfs_files)

$(BUILD_DIR)/$(INIT).bin: $(BUILD_DIR)/user/$(INIT).elf
	$(PROGRESS) "OBJCOPY" $@
	$(OBJCOPY) -j.initfs -j.text -j.data -j.rodata -j.bss -Obinary $< $@

$(BUILD_DIR)/kernel/__program_name.o:
	mkdir -p $(@D)
	echo "const char *__program_name(void) { return \"kernel\"; }" > $(@:.o=.c)
	$(CC) $(CFLAGS) -DKERNEL -c -o $@ $(@:.o=.c)

$(BUILD_DIR)/kernel/%.o: %.c Makefile
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/$(ARCH) -DKERNEL \
		-c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

$(BUILD_DIR)/kernel/%.o: %.S Makefile $(BUILD_DIR)/initfs.bin
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/$(ARCH) \
		-DKERNEL -DINITFS_BIN='"$(PWD)/$(BUILD_DIR)/initfs.bin"' \
		-c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

$(BUILD_DIR)/user/%.o: %.c Makefile
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Ilibs/resea -Ilibs/resea/arch/$(ARCH) \
		-c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

$(BUILD_DIR)/user/%.o: %.S Makefile
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Ilibs/resea -Ilibs/resea/arch/$(ARCH) \
		-c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

define lib-build-rule
CFLAGS += -Ilibs/$(1)/include
$(eval include libs/$(1)/lib.mk)
$(eval objs := $(addprefix $(BUILD_DIR)/user/libs/$(1)/, $(objs)))
$(eval $(BUILD_DIR)/user/libs/$(1).o: objs := $(objs))

$(BUILD_DIR)/user/libs/$(1).o: $(objs) Makefile
	$(PROGRESS) "LD" $(BUILD_DIR)/user/libs/$(1).o
	$(LD) -r -o $(BUILD_DIR)/user/libs/$(1).o $(objs)

-include $(BUILD_DIR)/user/libs/$(1)/*.deps
endef
$(foreach lib, $(libs), $(eval $(call lib-build-rule,$(lib))))

define server-build-rule
$(eval include servers/$(1)/build.mk)
$(eval objs := $(addprefix $(BUILD_DIR)/user/servers/$(1)/, $(objs)) $(lib_objs) \
	$(BUILD_DIR)/user/servers/$(1)/__program_name.o)
$(eval $(BUILD_DIR)/user/$(1).debug.elf: objs := $(objs))

$(BUILD_DIR)/user/servers/$(1)/__program_name.o:
	mkdir -p $(BUILD_DIR)/user/servers/$(1)
	echo "const char *__program_name(void) { return \"$(1)\"; }" > \
		$(BUILD_DIR)/user/servers/$(1)/__program_name.c
	$(CC) $(CFLAGS) -c \
		-o $(BUILD_DIR)/user/servers/$(1)/__program_name.o \
		$(BUILD_DIR)/user/servers/$(1)/__program_name.c

$(BUILD_DIR)/user/$(1).debug.elf: $(objs) tools/nm2symbols.py \
		tools/embed-symbols.py libs/resea/arch/$(ARCH)/user.ld Makefile
	$(PROGRESS) "LD" $(BUILD_DIR)/user/$(1).debug.elf
	$(LD) $(LDFLAGS) --script=libs/resea/arch/$(ARCH)/user.ld \
		-o $(BUILD_DIR)/user/$(1).elf.tmp $(objs)
	$(NM) $(BUILD_DIR)/user/$(1).elf.tmp | ./tools/nm2symbols.py > \
		$(BUILD_DIR)/user/$(1).symbols
	$(PROGRESS) "SYMBOLS" $(BUILD_DIR)/user/$(1).debug.elf
	./tools/embed-symbols.py $(BUILD_DIR)/user/$(1).symbols \
		$(BUILD_DIR)/user/$(1).elf.tmp
	mv $(BUILD_DIR)/user/$(1).elf.tmp $(BUILD_DIR)/user/$(1).debug.elf

$(BUILD_DIR)/user/$(1).elf: $(BUILD_DIR)/user/$(1).debug.elf
	$(PROGRESS) "STRIP" $(BUILD_DIR)/user/$(1).elf
	$(OBJCOPY) --strip-all-gnu --strip-debug \
		$(BUILD_DIR)/user/$(1).debug.elf $(BUILD_DIR)/user/$(1).elf

-include $(BUILD_DIR)/user/$(1)/servers/*.deps
endef
$(foreach server, $(INIT) $(SERVERS), $(eval $(call server-build-rule,$(server))))
$(foreach app, $(APPS), $(eval $(call server-build-rule,$(app))))

.config.mk:
	$(PROGRESS) "CONFIG" $@
	./tools/config.py --default

# JSON compilation database.
# https://clang.llvm.org/docs/JSONCompilationDatabase.html
$(BUILD_DIR)/compile_commands.json: $(kernel_objs)
	$(PROGRESS) "GEN" $(BUILD_DIR)/compile_commands.json
	$(PYTHON3) tools/merge-compile-commands.py \
		-o $(BUILD_DIR)/compile_commands.json \
		$(shell find $(BUILD_DIR)/kernel -type f -name "*.json") \
		$(shell find $(BUILD_DIR)/user -type f -name "*.json")

-include $(shell find $(BUILD_DIR) -name "*.deps" 2>/dev/null)
