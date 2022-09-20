name := resea
objs-y += printf.o syscall.o malloc.o init.o ipc.o async_ipc.o task.o
subdirs-y += arch/$(ARCH)

include $(top_dir)/mk/lib.mk
