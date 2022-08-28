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
objs := $(objs) $(foreach lib, $(libs), $(BUILD_DIR)/libs/$(lib).o)
ldflags := $(ldflags-y)

$(executable): LDFLAGS += $(ldflags)
$(executable): CFLAGS += $(foreach lib, $(libs), -Ilibs/$(lib)/include)
$(executable): OBJS := $(objs)
$(executable): $(objs)
	$(PROGRESS) LD $(@)
	mkdir -p $(@D)
	$(LD) $(LDFLAGS) -o $(@) $(OBJS)
