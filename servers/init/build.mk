objs-y += main.o bootfs.o bootfs_image.o
cflags-y += -DBOOTFS_PATH='"$(bootfs_bin)"'

$(build_dir)/bootfs_image.S: $(bootfs_bin)
	$(MKDIR) $(@D)
	echo ".section .rodata"      > $@
	echo ".global bootfs"        >> $@
	echo "bootfs:"               >> $@
	echo ".incbin BOOTFS_PATH"   >> $@

 include $(top_dir)/mk/executable.mk
