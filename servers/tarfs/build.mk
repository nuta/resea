name := tarfs
objs := main.o tarball.o

$(BUILD_DIR)/user/servers/tarfs/tarball.o: $(TARFS_TAR)
	if [ -z "$(TARFS_TAR)"  ]; then echo "$$(TARFS_TAR) is not defined."; exit 1; fi
	$(PROGRESS) "GEN" $@
	echo ".data; .align 4096; .global __tarball, __tarball_end; __tarball: .incbin \"$<\"; __tarball_end:" > $(@:.o=.S)
	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.S)

ifeq ($(shell uname),Darwin)
TAR ?= gtar
else
TAR = tar
endif
