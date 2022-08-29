executable_name := $(name)
build_dir := $(BUILD_DIR)/$(dir)
objs := $(addprefix $(build_dir)/, $(objs-y))

dir-saved = $(dir)
$(foreach subdir, $(subdirs-y),                                 \
	$(eval dir := $(dir-saved)/$(subdir))                   \
	$(eval build_dir := $(BUILD_DIR)/$(dir))                \
	$(eval objs-y :=)                                       \
	$(eval include $(dir)/build.mk)                         \
	$(eval objs += $(addprefix $(build_dir)/, $(objs-y)))   \
)

libs := $(libs-y)
objs := \
	$(objs) \
	$(foreach lib, $(libs), $(BUILD_DIR)/libs/$(lib).o) \
	$(BUILD_DIR)/program_names/$(executable_name).o
cflags := $(cflags-y)
ldflags := $(ldflags-y)

$(objs): CFLAGS := $(CFLAGS) $(cflags) $(foreach lib, $(libs), -Ilibs/$(lib)/include)
$(executable): LDFLAGS := $(LDFLAGS) $(ldflags)
$(executable): OBJS := $(objs)
$(executable): $(objs)
	$(PROGRESS) LD $(@)
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) -o $(@) $(OBJS)

$(BUILD_DIR)/program_names/$(executable_name).c:
	mkdir -p $(@D)
	echo 'const char *__program_name(void) { return "$(executable_name)"; }' > $(@)
