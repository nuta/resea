name := common
objs-y += list.o vprintf.o string.o ubsan.o
cflags-y += -I$(dir)/include

include $(top_dir)/mk/lib.mk
