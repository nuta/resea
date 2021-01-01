name := ramdisk
description := An ephemeral and pseudo block device driver
objs-y := main.o disk.o

RAM_DISK_IMG ?= $(BUILD_DIR)/ramdisk.img

$(BUILD_DIR)/servers/drivers/blk/ramdisk/disk.o: $(RAM_DISK_IMG)
	$(PROGRESS) "GEN" $@
	echo ".rodata; .align 4096; .global __image, __image_end; __image: .incbin \"$<\"; __image_end:" > $(@:.o=.S)
	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.S)

$(RAM_DISK_IMG):
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	dd if=/dev/zero of=$@.tmp bs=1024 count=4098
	mformat -i $@.tmp
	echo "Hello from FAT :D" > $(BUILD_DIR)/hello.txt
	mcopy -i $@.tmp $(BUILD_DIR)/hello.txt ::/HELLO.TXT
	mv $@.tmp $@
