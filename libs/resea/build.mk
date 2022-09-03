name := resea
objs-y += printf.o
subdirs-y += arch/$(ARCH)

include $(top_dir)/mk/lib.mk
