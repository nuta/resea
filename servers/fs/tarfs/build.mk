name := tarfs
objs-y := main.o tarball.o

$(BUILD_DIR)/servers/fs/tarfs/tarball.o:
	if [ -z "$(TARFS_TAR)"  ]; then echo "TARFS_TAR is not defined. Enable Linux ABI Compability Layer."; exit 1; fi
	make $(TARFS_TAR)
	$(PROGRESS) "GEN" $@
	mkdir -p $(dir $(@:.o=.S))
	echo ".data; .align 4096; .global __tarball, __tarball_end; __tarball: .incbin \"$(TARFS_TAR)\"; __tarball_end:" > $(@:.o=.S)
	$(CC) $(CFLAGS) -o $@ -c $(@:.o=.S)

ifeq ($(shell uname),Darwin)
TAR ?= gtar
else
TAR = tar
endif
