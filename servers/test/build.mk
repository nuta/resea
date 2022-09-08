objs-y += main.o
cflags-y += -DTEST

include $(top_dir)/mk/executable.mk
