objs-y += main.o
subdirs-y += arch/$(ARCH)

include $(top_dir)/mk/executable.mk
