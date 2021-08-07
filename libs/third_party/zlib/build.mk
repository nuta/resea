name := zlib
libs-y := third_party/libc
third-party-build := y
copyright :=
zlib_version := 1.2.11
zlib_tarball_url := https://download.sourceforge.net/sourceforge/libpng/zlib-$(zlib_version).tar.xz

zlib_dir := libs/third_party/zlib/zlib-$(zlib_version)

# Cflags for library users.
ZLIB_CFLAGS += -I$(zlib_dir)

objs-y := \
	zlib-$(zlib_version)/adler32.o \
	zlib-$(zlib_version)/crc32.o \
	zlib-$(zlib_version)/deflate.o \
	zlib-$(zlib_version)/infback.o \
	zlib-$(zlib_version)/inffast.o \
	zlib-$(zlib_version)/inflate.o \
	zlib-$(zlib_version)/inftrees.o \
	zlib-$(zlib_version)/trees.o \
	zlib-$(zlib_version)/zutil.o \
	zlib-$(zlib_version)/compress.o \
	zlib-$(zlib_version)/uncompr.o \
	zlib-$(zlib_version)/gzclose.o \
	zlib-$(zlib_version)/gzlib.o \
	zlib-$(zlib_version)/gzread.o \
	zlib-$(zlib_version)/gzwrite.o

autogen-files-y += $(zlib_dir)

$(addprefix $(BUILD_DIR)/libs/third_party/zlib/, $(objs-y)): CFLAGS += $(LIBC_CFLAGS) $(ZLIB_CFLAGS)

$(zlib_dir):
	$(PROGRESS) DOWNLOAD $(zlib_tarball_url)
	./tools/download-file.py --extract $(@D) $(zlib_tarball_url)
