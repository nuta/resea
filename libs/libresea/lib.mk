name := libresea
objs := printf.o string.o exit.o backtrace.o \
	syscall.o ubsan.o utils.o math.o malloc.o

include libs/libresea/arch/$(ARCH)/arch.mk
