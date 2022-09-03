build_dir := $(BUILD_DIR)/$(dir)
objs := $(addprefix $(build_dir)/, $(objs-y))

dir-saved := $(dir)
$(foreach subdir, $(subdirs-y),                                 \
	$(eval dir := $(dir-saved)/$(subdir))                   \
	$(eval build_dir := $(BUILD_DIR)/$(dir))                \
	$(eval objs-y :=)                                       \
	$(eval include $(dir)/build.mk)                         \
	$(eval objs += $(addprefix $(build_dir)/, $(objs-y)))   \
)

objs := $(objs)
cflags := $(cflags-y)

$(objs): CFLAGS := $(CFLAGS) $(cflags) -Ilibs/common/include -I$(dir-saved)/include
$(output): OBJS := $(objs)
$(output): $(objs)
	$(PROGRESS) LD $(@)
	$(MKDIR) $(@D)
	$(LD) -r -o $(@) $(OBJS)
