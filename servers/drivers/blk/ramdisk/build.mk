name := ramdisk
obj-y := main.o disk.o

$(BUILD_DIR)/servers/drivers/blk/ramdisk/disk.o: $(BUILD_DIR)/ramdisk.img
	$(PROGRESS) "GEN" $@
	echo ".data; .align 4096; .global __image, __image_end; __image: .incbin \"$<\"; __image_end:" > $(@:.o=.S)
	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.S)

$(BUILD_DIR)/ramdisk.img:
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	dd if=/dev/zero of=$@.tmp bs=1024 count=4098
	mformat -i $@.tmp
	echo "Hello from FAT :D" > $(BUILD_DIR)/hello.txt
	mcopy -i $@.tmp $(BUILD_DIR)/hello.txt ::/HELLO.TXT
	mv $@.tmp $@
