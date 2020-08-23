name := resea
obj-y += init.o printf.o malloc.o handle.o async.o task.o ipc.o timer.o
obj-y += cmdline.o
global-cflags-y += -I$(dir)/arch/$(ARCH)
subdir-y += arch/$(ARCH)
