name := resea
obj-y += init.o syscall.o printf.o malloc.o io.o lookup.o map.o handle.o
global-cflags-y += -I$(dir)/arch/$(ARCH)
subdir-y += arch/$(ARCH)
