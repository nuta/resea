objs-y += main.o
subdirs-y += arch/$(ARCH)
libs-y += common

include $(top_dir)/mk/executable.mk
