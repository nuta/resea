name := libresea
objs := printf.o arch/$(ARCH)/start_$(ARCH).o string.o exit.o backtrace.o \
	syscall.o ubsan.o utils.o math.o malloc.o

include libs/libresea/arch/$(ARCH)/arch.mk
