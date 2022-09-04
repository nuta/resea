objs-y += main.o printk.o memory.o task.o ipc.o syscall.o boot_elf.o
subdirs-y += arch/$(ARCH)
cflags-y += -DKERNEL -DBOOT_ELF_PATH='"$(boot_elf)"' -Ikernel/arch/$(ARCH)/include
libs-y += common
idls-y += messages.idl

$(build_dir)/boot_elf.S: $(boot_elf)
	$(MKDIR) $(@D)
	echo ".section .boot_elf"    > $@
	echo ".global boot_elf"      >> $@
	echo "boot_elf:"             >> $@
	echo ".incbin BOOT_ELF_PATH" >> $@

include $(top_dir)/mk/executable.mk
