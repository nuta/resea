name := ide
objs-y := main.o disk.o
libs-y := driver

QEMUFLAGS += -drive if=ide,file=$(BUILD_DIR)/ide.img

$(BUILD_DIR)/servers/drivers/blk/ide/main.o: $(ide_disk_dir)

$(BUILD_DIR)/servers/drivers/blk/ide/disk.o: $(BUILD_DIR)/ide.img
	$(PROGRESS) "GEN" $@
	echo ".data; .align 4096; .global __image, __image_end; __image: .incbin \"$<\"; __image_end:" > $(@:.o=.S)
	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.S)

$(BUILD_DIR)/ide.img:
	$(PROGRESS) "GEN" $@
	mkdir -p $(@D)
	dd if=/dev/zero of=$@.tmp bs=1024 count=4098
	mformat -i $@.tmp
	echo "Hello from FAT on IDE :D" > $(BUILD_DIR)/hello.txt
	mcopy -i $@.tmp $(BUILD_DIR)/hello.txt ::/HELLO.TXT
	mv $@.tmp $@
