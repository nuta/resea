objs-y += main.o printk.o memory.o task.o init.o
subdirs-y += arch/$(ARCH)
cflags-y += -DKERNEL -DBOOTELF_PATH='"$(BUILD_DIR)/servers/init.elf"' -Ikernel/arch/$(ARCH)/include
libs-y += common

$(build_dir)/init.S: $(BUILD_DIR)/servers/init.elf
	mkdir -p $(@D)
	echo ".rodata"                  > $@
	echo ".global __bootelf"        >> $@
	echo "__bootelf:"               >> $@
	echo "    .incbin BOOTELF_PATH" >> $@

include $(top_dir)/mk/executable.mk
