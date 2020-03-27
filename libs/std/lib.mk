objs := init.o syscall.o printf.o malloc.o io.o lookup.o session.o string.o map.o
include libs/std/arch/$(ARCH)/arch.mk
