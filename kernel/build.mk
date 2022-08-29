objs-y += main.o printk.o memory.o task.o bootelf.o
subdirs-y += arch/$(ARCH)
cflags-y += -DKERNEL -DBOOTELF_PATH='"$(boot_elf)"' -Ikernel/arch/$(ARCH)/include
libs-y += common

$(build_dir)/bootelf.S: $(boot_elf)
	mkdir -p $(@D)
	echo ".rodata"                  > $@
	echo ".global __bootelf"        >> $@
	echo "__bootelf:"               >> $@
	echo "    .incbin BOOTELF_PATH" >> $@

include $(top_dir)/mk/executable.mk
