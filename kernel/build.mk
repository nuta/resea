objs-y += main.o printk.o memory.o task.o
subdirs-y += arch/$(ARCH)
cflags-y += -DKERNEL -Ikernel/arch/$(ARCH)/include
libs-y += common

 include $(top_dir)/mk/executable.mk
