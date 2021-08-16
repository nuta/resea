name := gui
description := GUI window system
objs-y := main.o gui.o canvas.o
libs-y := ipc_client third_party/cairo

$(build_dir)/canvas.o: CFLAGS += $(LIBC_CFLAGS) $(CAIRO_CFLAGS)
