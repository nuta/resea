BUILD_DIR ?= build

# The default build target.
.PHONY: default
default: test

# Disable builtin implicit rules and variables.
MAKEFLAGS += --no-builtin-rules --no-builtin-variables
.SUFFIXES:

# Enable verbose output if $(V) is set.
ifeq ($(V),)
.SILENT:
endif

CC       := clang
PYTHON3  := python3
PROGRESS := printf "  \\033[1;96m%8s\\033[0m  \\033[1;m%s\\033[0m\\n"

executable := $(BUILD_DIR)/unittest/executable
autogen_files := $(BUILD_DIR)/unittest/include/idl.h

CFLAGS += -g3 -std=c11 -ffreestanding -fno-builtin
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
CFLAGS += -Ilibs/unittest/include/stubs -I$(BUILD_DIR)/unittest/include
CFLAGS += -fsanitize=address,undefined

# Visits the soruce directory recursively and fills $(cflags), $(objs) and $(libs).
# $(1): The target source dir.
# $(2): The build dir.
define visit-subdir
$(eval objs-y :=)
$(eval libs-y :=)
$(eval build_dir := $(2)/$(1))
$(eval subdirs-y :=)
$(eval cflags-y :=)
$(eval global-cflags-y :=)
$(eval include $(1)/build.mk)
$(eval build_mks += $(1)/build.mk)
$(eval objs += $(addprefix $(2)/$(1)/, $(objs-y)))
$(eval libs += $(libs-y))
$(eval cflags += $(cflags-y))
$(eval CFLAGS += $(global-cflags-y))
$(eval $(foreach subdir, $(subdirs-y), \
	$(eval $(call visit-subdir,$(1)/$(subdir),$(2)))))
endef

# Enumerate object files.
unittest_objs := runner.o stubs.o
objs := $(addprefix $(BUILD_DIR)/unittest/libs/unittest/, $(unittest_objs))
libs := common resea unittest
$(eval $(call visit-subdir,$(TARGET),$(BUILD_DIR)/unittest))
CFLAGS += $(foreach lib, $(libs), -Ilibs/$(lib)/include)
CFLAGS += -DPROGRAM_NAME='"$(name)"'
CFLAGS += -DUNITTEST_TARGET_NAME='"$(TARGET)"'

.PHONY: test
test: $(executable)
	$(PROGRESS) "RUN" $<
	UBSAN_OPTIONS=print_stacktrace=1 ./$<

$(executable): $(objs) $(BUILD_DIR)/unittest/unittest_funcs.o
	$(PROGRESS) "CC" $@
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/unittest/%.o: %.c $(autogen_files)
	$(PROGRESS) "CC" $<
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<
	$(CC) $(CFLAGS) -E -o - $< \
		| (grep "__unittest" || true) \
		>> $(BUILD_DIR)/unittest/unittest_funcs.txt

$(BUILD_DIR)/unittest/include/idl.h: tools/genidl.py $(wildcard *.idl */*.idl */*/*.idl)
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	./tools/genidl.py --idl interface.idl -o $@

$(BUILD_DIR)/unittest/unittest_funcs.o: $(BUILD_DIR)/unittest/unittest_funcs.c
	$(PROGRESS) "CC" $@
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/unittest/unittest_funcs.c: $(objs)
	$(PROGRESS) "GEN" $@
	$(PYTHON3) libs/unittest/extract-unittest-funcs.py  \
		$(BUILD_DIR)/unittest/unittest_funcs.txt > $@
