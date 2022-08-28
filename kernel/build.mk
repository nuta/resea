objs-y += main.o task.o
subdirs-y += arch/$(ARCH)
cflags-y += -Ikernel/arch/$(ARCH)/include
libs-y += common

 include $(top_dir)/mk/executable.mk
