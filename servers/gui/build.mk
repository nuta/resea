name := gui
description := GUI window system
objs-y := main.o gui.o canvas.o assets.o
libs-y := ipc_client assetfs third_party/cairo

roboto_url := https://github.com/googlefonts/roboto/releases/download/v2.138/roboto-android.zip
assets_build_dir := $(BUILD_DIR)/servers/gui/assets
icons := $(wildcard servers/gui/assets/*.svg)
png_icons := $(addprefix $(BUILD_DIR)/, $(icons:.svg=.png))
ICON_SIZE := 24

cflags-y += -DICON_SIZE=$(ICON_SIZE)

$(build_dir)/canvas.o: CFLAGS += $(LIBC_CFLAGS) $(CAIRO_CFLAGS)
$(build_dir)/assets.o: $(png_icons)
	$(PROGRESS) CURL $(roboto_url)
	mkdir -p $(build_dir) $(assets_build_dir)
	curl -sSL -o $(build_dir)/roboto.zip $(roboto_url)
	cd $(build_dir) && unzip -nq roboto.zip
	cp $(build_dir)/Roboto-Regular.ttf $(assets_build_dir)/ui-regular.ttf
	cp $(build_dir)/Roboto-Bold.ttf    $(assets_build_dir)/ui-bold.ttf

	$(PROGRESS) MKASSETFS $@
	./tools/mkassetfs.py $(assets_build_dir) -o $@.data
	$(OBJCOPY) -Ibinary -Oelf64-x86-64 $@.data $@

$(png_icons): $(BUILD_DIR)/%.png: %.svg
	$(PROGRESS) INKSCAPE $@
	mkdir -p $(@D)
	inkscape -d 96 -w $(ICON_SIZE) -h $(ICON_SIZE) -o $@ $<
