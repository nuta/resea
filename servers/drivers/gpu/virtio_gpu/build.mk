name := virtio_gpu
description := A virtio-gpu GPU driver
objs-y := main.o  cairo_demo.o
external-objs-y := $(addprefix $(BUILD_DIR)/libs/third_party/libc/, $(LIBC_ARCHIVES)) # FIXME:
libs-y := virtio driver third_party/cairo third_party/libc

$(build_dir)/cairo_demo.o: $(LIBC_SOUCE_DIRS) $(CAIRO_SOUCE_DIRS)

$(build_dir)/cairo_demo.o: CFLAGS   += $(LIBC_CFLAGS) $(CAIRO_CFLAGS)
$(build_dir)/cairo_demo.o: INCLUDES += $(LIBC_INCLUDES) $(CAIRO_INCLUDES)
