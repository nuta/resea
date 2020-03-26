name := ramdisk
objs := main.o disk.o

$(BUILD_DIR)/user/servers/ramdisk/disk.o: $(BUILD_DIR)/ramdisk.img
	$(PROGRESS) "GEN" $@
	echo ".data; .align 4096; .global __image, __image_end; __image: .incbin \"$<\"; __image_end:" > $(@:.o=.S)
	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.S)

$(BUILD_DIR)/ramdisk.img: $(foreach app, $(APPS), $(BUILD_DIR)/user/$(app).elf)
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	dd if=/dev/zero of=$@.tmp bs=1024 count=4098
	mformat -i $@.tmp
	mmd -i $@.tmp /apps
	for app in $(APPS); do mcopy -i $@.tmp $(BUILD_DIR)/user/$$app.elf ::/apps/$$app; done
	mv $@.tmp $@
