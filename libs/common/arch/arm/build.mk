# Because the ROM and RAM spaces are too small, we override some cflags
# to reduce the footprint.
global-cflags-y += -Oz -flto -fno-sanitize=undefined
