name := common
obj-y += string.o vprintf.o ubsan.o bitmap.o
subdir-y += arch/$(ARCH)
