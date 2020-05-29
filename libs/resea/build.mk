name := resea
obj-y += init.o syscall.o printf.o malloc.o io.o map.o handle.o
obj-y += task.o ipc.o timer.o klog.o
global-cflags-y += -I$(dir)/arch/$(ARCH)
subdir-y += arch/$(ARCH)
