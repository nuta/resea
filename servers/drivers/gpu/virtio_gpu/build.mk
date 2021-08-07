name := virtio_gpu
description := A virtio-gpu GPU driver
objs-y := main.o  cairo_demo.o roboto_font.o wallpaper.o
libs-y := virtio driver third_party/cairo

$(build_dir)/cairo_demo.o: CFLAGS := $(LIBC_CFLAGS) $(CAIRO_CFLAGS)
$(build_dir)/roboto_font.o:
	$(PROGRESS) DOWNLOAD roboto-android.zip
	curl -sSL -o $(build_dir)/roboto.zip https://github.com/googlefonts/roboto/releases/download/v2.138/roboto-android.zip
	cd $(build_dir) && unzip -nq roboto.zip
	$(PROGRESS) OBJCOPY $@
	$(OBJCOPY) -Ibinary -Oelf64-x86-64 $(build_dir)/Roboto-Regular.ttf $@

$(build_dir)/wallpaper.o: servers/drivers/gpu/virtio_gpu/wallpaper.png
	$(PROGRESS) OBJCOPY $@
	$(OBJCOPY) -Ibinary -Oelf64-x86-64 $< $@
