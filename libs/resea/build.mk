name := resea
objs-y += printf.o syscall.o malloc.o init.o
subdirs-y += arch/$(ARCH)

include $(top_dir)/mk/lib.mk
