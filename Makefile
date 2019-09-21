# Build options.
# Set 1 to enable verbose output.
V =

# The default build target.
.PHONY: default
default: build

# Enable verbose output if $(V) is set.
ifeq ($(V),)
.SILENT:
endif

# Determine if we need to load ".config".
non_config_targets := defconfig
load_config := y
ifeq ($(filter-out $(non_config_targets), $(MAKECMDGOALS)),)
load_config :=
endif
ifeq ($(MAKECMDGOALS),)
load_config := y
endif

# Include other makefiles.
ifeq ($(load_config), y)
ifeq ($(wildcard .config),)
$(error .config does not exist (run 'make defconfig' first))
endif

include .config
objs :=
include kernel/arch/$(ARCH)/arch.mk
include kernel/kernel.mk
kernel_objs := $(addprefix $(BUILD_DIR)/, $(objs)) $(BUILD_DIR)/initfs.o
include libs/libruntime/user_$(ARCH).mk
endif

# Disable builtin implicit rules and variables.
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

# Commands.
CC           := $(LLVM_PREFIX)clang
LD           := $(LLVM_PREFIX)ld.lld
NM           := $(LLVM_PREFIX)llvm-nm
OBJCOPY      := $(LLVM_PREFIX)llvm-objcopy
CLANG_TIDY   := $(LLVM_PREFIX)clang-tidy
PROGRESS     := printf "  \\033[1;35m%8s  \\033[1;m%s\\033[m\\n"

# LLD options.
KERNEL_LDFLAGS += --script=kernel/arch/$(ARCH)/$(ARCH).ld

ifeq ($(BUILD), debug)
COMMON_CFLAGS += -O1 -fsanitize=undefined -DDEBUG_BUILD
else
COMMON_CFLAGS += -O3 -DRELEASE_BUILD
endif

# Clang options.
COMMON_CFLAGS += -std=c11 -ffreestanding -fno-builtin -nostdlib
COMMON_CFLAGS += -fno-omit-frame-pointer
COMMON_CFLAGS += -g3 -fstack-size-section
COMMON_CFLAGS += -Wall -Wextra
COMMON_CFLAGS += -Werror=implicit-function-declaration
COMMON_CFLAGS += -Werror=int-conversion
COMMON_CFLAGS += -Werror=incompatible-pointer-types
COMMON_CFLAGS += -Werror=shift-count-overflow
COMMON_CFLAGS += -Werror=return-type
COMMON_CFLAGS += -Ilibs/libcommon/include
KERNEL_CFLAGS += -DKERNEL -Ikernel/include -Ikernel/arch/$(ARCH)/include
KERNEL_CFLAGS += -I$(BUILD_DIR)/include
KERNEL_CFLAGS += $(COMMON_CFLAGS)
APP_CFLAGS += $(COMMON_CFLAGS) -I$(BUILD_DIR)/include

# Commands.
.PHONY: build
build:
	$(MAKE) $(BUILD_DIR)/kernel.elf
	$(MAKE) compile_commands

.PHONY: clean
clean:
	rm -r $(BUILD_DIR)

.PHONY: defconfig
defconfig:
	$(PYTHON3) tools/buildconfig.py default
	echo "Generated .config"

.PHONY: docs
docs:
	doxygen misc/Doxyfile
	echo "*** Generated build/srcdocs"

.PHONY: info
info:
	$(PYTHON3) -m platform
	$(CC) --version | head -n1
	$(LD) --version
	$(PYTHON3) --version

.PHONY: lint
lint:
	$(CLANG_TIDY) -p $(BUILD_DIR) $(shell find . -type f -name "*.[ch]")

# The kernel executable.
$(kernel_objs): CFLAGS := $(KERNEL_CFLAGS)
$(BUILD_DIR)/kernel.elf: $(kernel_objs) kernel/arch/$(ARCH)/$(ARCH).ld tools/link.py Makefile
	$(PROGRESS) "LINK" $@
	$(PYTHON3) tools/link.py                      \
		--cc="$(CC)"                          \
		--cflags="$(KERNEL_CFLAGS)"           \
		--ld="$(LD)"                          \
		--ldflags="$(KERNEL_LDFLAGS)"         \
		--nm="$(NM)"                          \
		--build-dir="$(BUILD_DIR)"            \
		--mapfile="$(BUILD_DIR)/kernel.map"   \
		--outfile="$@"                        \
		$(kernel_objs)

# IDL stub.
$(BUILD_DIR)/include/resea_idl.h: misc/interfaces.idl tools/genstub.py
	$(PROGRESS) "GENSTUB" $@
	mkdir -p $(@D)
	$(PYTHON3) tools/genstub.py $< > $@

# Define server build rules.
define server-make-rule
$(eval include servers/$(1)/server.mk)
$(eval server_objs := $(addprefix $(BUILD_DIR)/servers/$(1)/, $(objs)))
$(eval server_libs := libruntime $(libs))
$(eval lib_objs := $(foreach lib, $(server_libs), $(BUILD_DIR)/libs/$(lib).lib.o))
$(server_objs): CFLAGS := -DPROGRAM_NAME='"$(1)"' $(APP_CFLAGS) \
	$(foreach lib, $(server_libs), -Ilibs/$(lib)/include)
$(BUILD_DIR)/servers/$(1).elf: $(server_objs) $(lib_objs) servers/$(1)/server.mk
$(BUILD_DIR)/servers/$(1).elf: server_name := $(1)
# Use server's own linker script if exists.
$(BUILD_DIR)/servers/$(1).elf: ldflags := \
	$(if $(wildcard servers/$(1)/$(1)_$(ARCH).ld), \
		--script=servers/$(1)/$(1)_$(ARCH).ld, \
		--script=libs/libruntime/user_$(ARCH).ld)
$(BUILD_DIR)/servers/$(1).elf: objs := $(server_objs) $(lib_objs)
lib_deps += $(filter-out $(lib_deps), $(server_libs))
endef
$(foreach server, $(INIT) $(STARTUPS), $(eval $(call server-make-rule,$(server))))
initfs_files += $(foreach server, $(STARTUPS), $(BUILD_DIR)/initfs/startups/$(server).elf)

# Define app build rules.
define app-make-rule
$(eval include apps/$(1)/app.mk)
$(eval app_objs := $(addprefix $(BUILD_DIR)/apps/$(1)/, $(objs)))
$(eval app_libs := libruntime $(libs))
$(eval lib_objs := $(foreach lib, $(app_libs), $(BUILD_DIR)/libs/$(lib).lib.o))
$(app_objs): CFLAGS := -DPROGRAM_NAME='"$(1)"' $(APP_CFLAGS) \
	$(foreach lib, $(app_libs), -Ilibs/$(lib)/include)
$(BUILD_DIR)/apps/$(1).elf: $(app_objs) $(lib_objs) apps/$(1)/app.mk
$(BUILD_DIR)/apps/$(1).elf: app_name := $(1)
# Use app's own linker script if exists.
$(BUILD_DIR)/apps/$(1).elf: ldflags := \
	$(if $(wildcard apps/$(1)/$(1)_$(ARCH).ld), \
		--script=apps/$(1)/$(1)_$(ARCH).ld, \
		--script=libs/libruntime/user_$(ARCH).ld)
$(BUILD_DIR)/apps/$(1).elf: objs := $(app_objs) $(lib_objs)
lib_deps += $(filter-out $(lib_deps), $(app_libs))
endef
$(foreach app, $(APPS), $(eval $(call app-make-rule,$(app))))
initfs_files += $(foreach app, $(APPS), $(BUILD_DIR)/initfs/apps/$(app).elf)

# Define library build rules.
define lib-make-rule
$(eval include libs/$(1)/lib.mk)
$(eval lib_objs := $(addprefix $(BUILD_DIR)/libs/$(1)/, $(objs)))
$(BUILD_DIR)/libs/$(1).lib.o: $(lib_objs) libs/$(1)/lib.mk
$(BUILD_DIR)/libs/$(1).lib.o: objs := $(lib_objs)
$(lib_objs): CFLAGS := $(APP_CFLAGS) \
	$(foreach lib, $(1) $(libs), -Ilibs/$(lib)/include)
endef
$(foreach lib, $(lib_deps), $(eval $(call lib-make-rule,$(lib))))

# Initfs.
$(BUILD_DIR)/initfs.o: $(initfs_files) $(BUILD_DIR)/$(INIT).bin tools/mkinitfs.py Makefile
	$(PROGRESS) "MKINITFS" $@
	$(PYTHON3) tools/mkinitfs.py -s $(BUILD_DIR)/$(INIT).bin \
		--generate-asm $(@:.o=.S) -o $(@:.o=.bin) $(BUILD_DIR)/initfs
	$(CC) $(KERNEL_CFLAGS) -c -o $@ $(@:.o=.S)

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

# C/assembly source file build rules.
$(BUILD_DIR)/%.o: %.c Makefile $(BUILD_DIR)/include/resea_idl.h
	$(PROGRESS) "CC" $@
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json) -c -o $@ $<

$(BUILD_DIR)/%.o: %.S Makefile
	$(PROGRESS) "CC" $@
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json) -c -o $@ $<

# Server executables.
$(BUILD_DIR)/servers/%.elf:
	$(PROGRESS) "LINK" $@
	$(PYTHON3) tools/link.py                              \
		--cc="$(CC)"                                  \
		--cflags="$(APP_CFLAGS)"                      \
		--ld="$(LD)"                                  \
		--ldflags="$(ldflags)"                        \
		--nm="$(NM)"                                  \
		--build-dir="$(basename $@)"                  \
		--mapfile="$(basename $@)/$(server_name).map" \
		--outfile="$@"                                \
		$(objs)

# App executables.
$(BUILD_DIR)/apps/%.elf:
	$(PROGRESS) "LINK" $@
	$(PYTHON3) tools/link.py                           \
		--cc="$(CC)"                               \
		--cflags="$(APP_CFLAGS)"                   \
		--ld="$(LD)"                               \
		--ldflags="$(ldflags)"                     \
		--nm="$(NM)"                               \
		--build-dir="$(basename $@)"               \
		--mapfile="$(basename $@)/$(app_name).map" \
		--outfile="$@"                             \
		$(objs)

# Libraries.
$(BUILD_DIR)/libs/%.lib.o:
	$(PROGRESS) "LD" $@
	mkdir -p $(@D)
	$(LD) -r -o $@ $(objs)

# The JSON compilation database.
# https://clang.llvm.org/docs/JSONCompilationDatabase.html
.PHONY: compile_commands
compile_commands:
	$(PROGRESS) "GEN" $(BUILD_DIR)/compile_commands.json
	$(PYTHON3) tools/merge-compile-commands.py \
		-o $(BUILD_DIR)/compile_commands.json \
		$(shell find $(BUILD_DIR) -type f -name "*.json" \
			| grep -v compile_commands.json)

# Include header file dependencies generated by clang.
-include $(BUILD_DIR)/kernel/*.deps
-include $(BUILD_DIR)/kernel/arch/$(ARCH)/*.deps
-include $(foreach app, $(APPS), $(BUILD_DIR)/apps/$(app)/*.deps)
-include $(foreach server, $(STARTUPS) $(INIT), $(BUILD_DIR)/server/$(server)/*.deps)
-include $(foreach lib, $(lib_deps), $(BUILD_DIR)/libs/$(lib)/*.deps)
