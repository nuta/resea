name := resea
objs-y += init.o printf.o malloc.o handle.o async.o task.o ipc.o timer.o mymalloc.o
objs-y += cmdline.o
global-cflags-y += -I$(dir)/arch/$(ARCH)
subdirs-y += arch/$(ARCH)
