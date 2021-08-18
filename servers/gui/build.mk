name := gui
description := GUI window system
objs-y := main.o gui.o canvas.o icons.o
libs-y := ipc_client assetfs third_party/cairo

$(build_dir)/canvas.o: CFLAGS += $(LIBC_CFLAGS) $(CAIRO_CFLAGS)
$(build_dir)/icons.o: servers/gui/icons/*.png
	$(PROGRESS) MKASSETFS $@
	./tools/mkassetfs.py servers/gui/icons -o $@.data
	$(OBJCOPY) -Ibinary -Oelf64-x86-64 $@.data $@
