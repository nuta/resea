objs := init.o syscall.o printf.o malloc.o io.o lookup.o string.o map.o rand.o
include libs/std/arch/$(ARCH)/arch.mk
