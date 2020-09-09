name := minlin
obj-y := main.o proc.o mm.o fs.o tty.o syscall.o waitqueue.o
libs-y := driver

$(BUILD_DIR)/minlin.tar:
	mkdir -p $(BUILD_DIR)
	cd servers/minlin && \
		./build.py \
			--build-dir $(abspath $(BUILD_DIR)/minlin-root) \
			-o $(abspath $@)

TARFS_TAR = $(BUILD_DIR)/minlin.tar
