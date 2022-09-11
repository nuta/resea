name := common
objs-y += list.o vprintf.o string.o ubsan.o error.o
subdirs-y += arch/$(ARCH)
cflags-y += -I$(dir)/include

include $(top_dir)/mk/lib.mk
