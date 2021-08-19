name := gui
description := GUI window system
objs-y := main.o gui.o canvas.o assets.o
libs-y := ipc_client assetfs third_party/cairo

roboto_url := https://github.com/googlefonts/roboto/releases/download/v2.138/roboto-android.zip

$(build_dir)/canvas.o: CFLAGS += $(LIBC_CFLAGS) $(CAIRO_CFLAGS)
$(build_dir)/assets.o: servers/gui/assets/*
	$(PROGRESS) CURL $(roboto_url)
	mkdir -p $(build_dir)
	curl -sSL -o $(build_dir)/roboto.zip $(roboto_url)
	cd $(build_dir) && unzip -nq roboto.zip
	cp $(build_dir)/Roboto-Regular.ttf servers/gui/assets/ui-regular.ttf
	cp $(build_dir)/Roboto-Bold.ttf servers/gui/assets/ui-bold.ttf

	$(PROGRESS) MKASSETFS $@
	./tools/mkassetfs.py servers/gui/assets -o $@.data
	$(OBJCOPY) -Ibinary -Oelf64-x86-64 $@.data $@
