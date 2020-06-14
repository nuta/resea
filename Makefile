# Default values for build system.
export V         ?=
export VERSION   ?= v0.2.0
export BUILD_DIR ?= build
ifeq ($(shell uname), Darwin)
LLVM_PREFIX ?= /usr/local/opt/llvm/bin/
GRUB_PREFIX ?= /usr/local/opt/i386-elf-grub/bin/i386-elf-
endif

# The default build target.
.PHONY: default
default: build

# Disable builtin implicit rules and variables.
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

# Enable verbose output if $(V) is set.
ifeq ($(V),)
.SILENT:
endif

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
ifeq ($(wildcard .config),)
$(error .config does not exist (run 'make menuconfig' first))
endif

include .config

export ARCH  := $(patsubst "%",%,$(CONFIG_ARCH))
bootstrap    := $(patsubst "%",%,$(CONFIG_BOOTSTRAP))
kernel_image := $(BUILD_DIR)/resea.elf
bootfs_bin   := $(BUILD_DIR)/bootfs.bin
builtin_libs := common resea
all_servers  := $(notdir $(wildcard servers/*))
all_libs     := $(notdir $(wildcard libs/*))
servers      := \
	$(sort $(foreach server, $(all_servers), \
		$(if $(value $(shell echo CONFIG_$(server)_SERVER | \
			tr  '[:lower:]' '[:upper:]')), $(server),)))
autostarts   := \
	$(sort $(foreach server, $(all_servers), \
		$(if $(filter-out m, $(value $(shell echo CONFIG_$(server)_SERVER | \
			tr  '[:lower:]' '[:upper:]'))), $(server),)))
bootfs_files  := $(foreach name, $(servers), $(BUILD_DIR)/$(name).elf)
autogen_files := $(BUILD_DIR)/include/config.h $(BUILD_DIR)/include/idl.h

# Visits the soruce directory recursively and fills $(cflags), $(objs) and $(libs).
# $(1): The target source dir.
# $(2): The build dir.
define visit-subdir
$(eval obj-y :=)
$(eval lib-y :=)
$(eval build_dir := $(2)/$(1))
$(eval subdir-y :=)
$(eval cflags-y :=)
$(eval global-cflags-y :=)
$(eval include $(1)/build.mk)
$(eval build_mks += $(1)/build.mk)
$(eval objs += $(addprefix $(2)/$(1)/, $(obj-y)))
$(eval libs += $(lib-y))
$(eval cflags += $(cflags-y))
$(eval CFLAGS += $(global-cflags-y))
$(eval $(foreach subdir, $(subdir-y), \
	$(eval $(call visit-subdir,$(1)/$(subdir),$(2)))))
endef

# Enumerate kernel object files.
objs :=
$(eval $(call visit-subdir,kernel,$(BUILD_DIR)/kernel))
$(eval $(call visit-subdir,libs/common,$(BUILD_DIR)/kernel))
kernel_objs := $(objs)

endif # ifeq ($(load_config), y)

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
CFLAGS += -Wno-unused-parameter
CFLAGS += -fstack-size-section
CFLAGS += -Ilibs/common/include -Ilibs/common/arch/$(ARCH)
CFLAGS += -I$(BUILD_DIR)/include
CFLAGS += -DVERSION='"$(VERSION)"'
CFLAGS += -DBOOTELF_PATH='"$(BUILD_DIR)/$(bootstrap).elf"'
CFLAGS += -DBOOTFS_PATH='"$(bootfs_bin)"'
CFLAGS += -DAUTOSTARTS='"$(autostarts)"'

ifdef CONFIG_BUILD_RELEASE
CFLAGS += -O2 -flto
else
CFLAGS += -O1 -fsanitize=undefined
endif

#
#  Build Commands
#
.PHONY: build
build: $(kernel_image) $(BUILD_DIR)/compile_commands.json

.PHONY: lint
lint: $(BUILD_DIR)/compile_commands.json
	find kernel servers libs -name "*.c" | xargs $(CLANG_TIDY) -p $(BUILD_DIR)

.PHONY: test
test: $(kernel_image)
	./tools/run-and-check.py $(RUNCHECKFLAGS) "Passed all tests" -- $(MAKE) run

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: defconfig
defconfig:
	$(PROGRESS) "CONFIG"
	./tools/config.py --defconfig

.PHONY: menuconfig
menuconfig:
	./tools/config.py --menuconfig

#
#  Build Rules
#
$(kernel_image): $(kernel_objs) $(BUILD_DIR)/kernel/__name__.o \
		kernel/arch/$(ARCH)/kernel.ld \
		tools/nm2symbols.py tools/embed-symbols.py Makefile
	$(PROGRESS) "LD" $@
	$(LD) $(LDFLAGS) --script=kernel/arch/$(ARCH)/kernel.ld \
		-Map $(@:.elf=.map) -o $@.tmp \
		$(kernel_objs) $(BUILD_DIR)/kernel/__name__.o
	$(PROGRESS) "GEN" $(@:.elf=.symbols)
	$(NM) $@.tmp | ./tools/nm2symbols.py > $(@:.elf=.symbols)
	$(PROGRESS) "SYMBOLS" $@
	./tools/embed-symbols.py $(@:.elf=.symbols) $@.tmp
	cp $@.tmp $@

$(bootfs_bin): $(bootfs_files) tools/mkbootfs.py
	$(PROGRESS) "MKBOOTFS" $@
	$(PYTHON3) tools/mkbootfs.py -o $@ $(bootfs_files)

$(BUILD_DIR)/kernel/%.o: %.c Makefile $(autogen_files)
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/$(ARCH) -DKERNEL \
		-c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

$(BUILD_DIR)/kernel/%.o: %.S Makefile $(BUILD_DIR)/$(bootstrap).elf $(autogen_files)
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -Ikernel -Ikernel/arch/$(ARCH) -DKERNEL \
		-c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

$(BUILD_DIR)/kernel/__name__.c: $(autogen_files)
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	echo "const char *__program_name(void) { return \"kernel\"; }" > $(@)

$(BUILD_DIR)/kernel/__name__.o: $(BUILD_DIR)/kernel/__name__.c
	$(PROGRESS) "CC" $<
	$(CC) $(CFLAGS) -DKERNEL -c -o $@ $(@:.o=.c)

#
#  Userland build rules
#
$(BUILD_DIR)/%.o: %.c Makefile $(autogen_files)
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

$(BUILD_DIR)/%.o: %.c Makefile $(autogen_files)
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

$(BUILD_DIR)/%.o: %.S Makefile $(autogen_files)
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

#
#  Auto-generated files.
#
$(BUILD_DIR)/include/config.h: .config tools/config.py
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	./tools/config.py --genconfig $@

$(BUILD_DIR)/include/idl.h: tools/genidl.py $(wildcard *.idl */*.idl */*/*.idl)
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	./tools/genidl.py --idl interface.idl -o $@

# JSON compilation database.
# https://clang.llvm.org/docs/JSONCompilationDatabase.html
$(BUILD_DIR)/compile_commands.json: $(kernel_objs)
	$(PROGRESS) "GEN" $(BUILD_DIR)/compile_commands.json
	$(PYTHON3) tools/merge-compile-commands.py \
		-o $(BUILD_DIR)/compile_commands.json \
		$(shell find $(BUILD_DIR) -type f -name "*.json")

#
#  Libs build rules
#
define lib-build-rule
$(eval dir := libs/$(1))
$(eval outfile := $(BUILD_DIR)/libs/$(1).lib.o)
$(eval name :=)
$(eval objs :=)
$(eval cflags :=)
$(eval build_mks :=)
$(eval $(call visit-subdir,libs/$(1),$(BUILD_DIR)))
$(eval $(outfile): objs := $(objs))
$(eval $(outfile): $(objs) $(build_mks))
$(eval $(objs): CFLAGS += $(cflags))
endef
$(foreach lib, $(all_libs), \
	$(eval $(call lib-build-rule,$(lib))))

$(BUILD_DIR)/libs/%.lib.o:
	$(PROGRESS) "LD" $(@)
	$(LD) -r -o $(@) $(objs)

#
#  Server build rules
#
define server-build-rule
$(eval dir := servers/$(1))
$(eval name :=)
$(eval objs :=)
$(eval libs := $(builtin_libs))
$(eval cflags :=)
$(eval build_mks :=)
$(eval objs += $(BUILD_DIR)/servers/$(1)/__name__.o)
$(eval objs += $(foreach lib, $(libs), $(BUILD_DIR)/libs/$(lib).lib.o))
$(eval $(call visit-subdir,$(dir),$(BUILD_DIR)))
$(eval $(BUILD_DIR)/servers/$(1)/__name__.c: name := $(name))
$(eval $(BUILD_DIR)/$(1).elf: name := $(name))
$(eval $(BUILD_DIR)/$(1).debug.elf: name := $(name))
$(eval $(BUILD_DIR)/$(1).debug.elf: objs := $(objs))
$(eval $(BUILD_DIR)/$(1).debug.elf: $(objs) $(build_mks))
$(eval $(objs): CFLAGS += $(cflags))
$(foreach lib, $(all_libs),
	$(eval $(objs): CFLAGS += -Ilibs/$(lib)/include))
endef
$(foreach server, $(bootstrap) $(servers), \
	$(eval $(call server-build-rule,$(server))))

%/__name__.c:
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	echo "const char *__program_name(void) { return \"$(name)\"; }" > $(@)

%/__name__.o: %/__name__.c $(autogen_files)
	$(PROGRESS) "CC" $@
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $(@) $<

$(BUILD_DIR)/%.debug.elf: tools/nm2symbols.py \
		tools/embed-symbols.py libs/resea/arch/$(ARCH)/user.ld Makefile
	$(PROGRESS) "LD" $(@)
	$(LD) $(LDFLAGS) --script=libs/resea/arch/$(ARCH)/user.ld \
		-Map $(BUILD_DIR)/$(1).map -o $(@).tmp $(objs)
	$(NM) $(@).tmp | ./tools/nm2symbols.py > $(BUILD_DIR)/$(name).symbols
	$(PROGRESS) "SYMBOLS" $(BUILD_DIR)/$(name).debug.elf
	./tools/embed-symbols.py $(BUILD_DIR)/$(name).symbols $(@).tmp
	mv $(@).tmp $@

$(BUILD_DIR)/%.elf: $(BUILD_DIR)/%.debug.elf
	$(PROGRESS) "STRIP" $@
	$(OBJCOPY) --strip-all-gnu --strip-debug $< $@
	./tools/embed-server-name.py --name=$(name) $(@)

-include $(shell find $(BUILD_DIR) -name "*.deps" 2>/dev/null)
