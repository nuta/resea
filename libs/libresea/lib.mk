name := libresea
objs := printf.o exit.o backtrace.o \
	syscall.o ubsan.o utils.o math.o \
	malloc.o vector.o hash.o string.o

include libs/libresea/arch/$(ARCH)/arch.mk
