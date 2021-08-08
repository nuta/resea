name := cairo
libs-y := third_party/libc third_party/zlib
third-party-build := y
copyright := See https://github.com/freedesktop/cairo/blob/master/COPYING and https://github.com/freedesktop/pixman/blob/master/COPYING

cairo_version := 1.17.4
pixman_version := 0.40.0
libpng_version := 1.6.37
freetype_version := 2.11.0
cairo_tarball_url := https://cairographics.org/snapshots/cairo-$(cairo_version).tar.xz
pixman_tarball_url := https://cairographics.org/releases/pixman-$(pixman_version).tar.gz
libpng_tarball_url := https://download.sourceforge.net/sourceforge/libpng/libpng-$(libpng_version).tar.xz
freetype_tarball_url := https://download.savannah.gnu.org/releases/freetype/freetype-$(freetype_version).tar.xz

cairo_dir := libs/third_party/cairo/cairo-$(cairo_version)
pixman_dir := libs/third_party/cairo/pixman-$(pixman_version)
libpng_dir := libs/third_party/cairo/libpng-$(libpng_version)
freetype_dir := libs/third_party/cairo/freetype-$(freetype_version)

# Cflags for library users.
CAIRO_CFLAGS += -DHAVE_STDINT_H -DHAVE_UINT64_T -DHAVE_INTTYPES_H
CAIRO_CFLAGS += -DHAVE_STDLIB_H -DHAVE_STRINGS_H -DHAVE_STRING_H -DHAVE_MEMORY_H
CAIRO_CFLAGS += -DPIXMAN_NO_TLS -DPACKAGE
CAIRO_CFLAGS += -Ilibs/third_party/cairo/internal_headers
CAIRO_CFLAGS += -I$(cairo_dir)/src
CAIRO_CFLAGS += -I$(pixman_dir)/pixman
CAIRO_CFLAGS += -I$(libpng_dir)
CAIRO_CFLAGS += -I$(freetype_dir)/include

cflags-y += $(CAIRO_CFLAGS)
cflags-y += -Wno-return-type -Wno-missing-braces -Wno-enum-conversion
cflags-y += -Wno-missing-field-initializers -Wno-unused
cflags-y += -Wno-incompatible-pointer-types
cflags-y += -Wno-parentheses-equality
autogen-files-y += $(cairo_dir) $(pixman_dir) $(libpng_dir) $(freetype_dir)

#
#  Internal build rules
#

cairo_sources = \
	cairo-$(cairo_version)/src/cairo-analysis-surface.c \
	cairo-$(cairo_version)/src/cairo-arc.c \
	cairo-$(cairo_version)/src/cairo-array.c \
	cairo-$(cairo_version)/src/cairo-atomic.c \
	cairo-$(cairo_version)/src/cairo-base64-stream.c \
	cairo-$(cairo_version)/src/cairo-base85-stream.c \
	cairo-$(cairo_version)/src/cairo-bentley-ottmann-rectangular.c \
	cairo-$(cairo_version)/src/cairo-bentley-ottmann-rectilinear.c \
	cairo-$(cairo_version)/src/cairo-bentley-ottmann.c \
	cairo-$(cairo_version)/src/cairo-botor-scan-converter.c \
	cairo-$(cairo_version)/src/cairo-boxes-intersect.c \
	cairo-$(cairo_version)/src/cairo-boxes.c \
	cairo-$(cairo_version)/src/cairo-cache.c \
	cairo-$(cairo_version)/src/cairo-clip-boxes.c \
	cairo-$(cairo_version)/src/cairo-clip-polygon.c \
	cairo-$(cairo_version)/src/cairo-clip-region.c \
	cairo-$(cairo_version)/src/cairo-clip-surface.c \
	cairo-$(cairo_version)/src/cairo-clip-tor-scan-converter.c \
	cairo-$(cairo_version)/src/cairo-clip.c \
	cairo-$(cairo_version)/src/cairo-color.c \
	cairo-$(cairo_version)/src/cairo-composite-rectangles.c \
	cairo-$(cairo_version)/src/cairo-compositor.c \
	cairo-$(cairo_version)/src/cairo-contour.c \
	cairo-$(cairo_version)/src/cairo-damage.c \
	cairo-$(cairo_version)/src/cairo-debug.c \
	cairo-$(cairo_version)/src/cairo-default-context.c \
	cairo-$(cairo_version)/src/cairo-device.c \
	cairo-$(cairo_version)/src/cairo-error.c \
	cairo-$(cairo_version)/src/cairo-fallback-compositor.c \
	cairo-$(cairo_version)/src/cairo-fixed.c \
	cairo-$(cairo_version)/src/cairo-font-face-twin-data.c \
	cairo-$(cairo_version)/src/cairo-font-face-twin.c \
	cairo-$(cairo_version)/src/cairo-font-face.c \
	cairo-$(cairo_version)/src/cairo-font-options.c \
	cairo-$(cairo_version)/src/cairo-freed-pool.c \
	cairo-$(cairo_version)/src/cairo-freelist.c \
	cairo-$(cairo_version)/src/cairo-gstate.c \
	cairo-$(cairo_version)/src/cairo-hash.c \
	cairo-$(cairo_version)/src/cairo-hull.c \
	cairo-$(cairo_version)/src/cairo-image-compositor.c \
	cairo-$(cairo_version)/src/cairo-image-info.c \
	cairo-$(cairo_version)/src/cairo-image-source.c \
	cairo-$(cairo_version)/src/cairo-image-surface.c \
	cairo-$(cairo_version)/src/cairo-line.c \
	cairo-$(cairo_version)/src/cairo-lzw.c \
	cairo-$(cairo_version)/src/cairo-mask-compositor.c \
	cairo-$(cairo_version)/src/cairo-matrix.c \
	cairo-$(cairo_version)/src/cairo-mempool.c \
	cairo-$(cairo_version)/src/cairo-mesh-pattern-rasterizer.c \
	cairo-$(cairo_version)/src/cairo-misc.c \
	cairo-$(cairo_version)/src/cairo-mono-scan-converter.c \
	cairo-$(cairo_version)/src/cairo-mutex.c \
	cairo-$(cairo_version)/src/cairo-no-compositor.c \
	cairo-$(cairo_version)/src/cairo-observer.c \
	cairo-$(cairo_version)/src/cairo-output-stream.c \
	cairo-$(cairo_version)/src/cairo-paginated-surface.c \
	cairo-$(cairo_version)/src/cairo-path-bounds.c \
	cairo-$(cairo_version)/src/cairo-path-fill.c \
	cairo-$(cairo_version)/src/cairo-path-fixed.c \
	cairo-$(cairo_version)/src/cairo-path-in-fill.c \
	cairo-$(cairo_version)/src/cairo-path-stroke-boxes.c \
	cairo-$(cairo_version)/src/cairo-path-stroke-polygon.c \
	cairo-$(cairo_version)/src/cairo-path-stroke-traps.c \
	cairo-$(cairo_version)/src/cairo-path-stroke-tristrip.c \
	cairo-$(cairo_version)/src/cairo-path-stroke.c \
	cairo-$(cairo_version)/src/cairo-path.c \
	cairo-$(cairo_version)/src/cairo-pattern.c \
	cairo-$(cairo_version)/src/cairo-pen.c \
	cairo-$(cairo_version)/src/cairo-polygon-intersect.c \
	cairo-$(cairo_version)/src/cairo-polygon-reduce.c \
	cairo-$(cairo_version)/src/cairo-polygon.c \
	cairo-$(cairo_version)/src/cairo-raster-source-pattern.c \
	cairo-$(cairo_version)/src/cairo-recording-surface.c \
	cairo-$(cairo_version)/src/cairo-rectangle.c \
	cairo-$(cairo_version)/src/cairo-rectangular-scan-converter.c \
	cairo-$(cairo_version)/src/cairo-region.c \
	cairo-$(cairo_version)/src/cairo-rtree.c \
	cairo-$(cairo_version)/src/cairo-scaled-font.c \
	cairo-$(cairo_version)/src/cairo-shape-mask-compositor.c \
	cairo-$(cairo_version)/src/cairo-slope.c \
	cairo-$(cairo_version)/src/cairo-spans-compositor.c \
	cairo-$(cairo_version)/src/cairo-spans.c \
	cairo-$(cairo_version)/src/cairo-spline.c \
	cairo-$(cairo_version)/src/cairo-stroke-dash.c \
	cairo-$(cairo_version)/src/cairo-stroke-style.c \
	cairo-$(cairo_version)/src/cairo-surface-clipper.c \
	cairo-$(cairo_version)/src/cairo-surface-fallback.c \
	cairo-$(cairo_version)/src/cairo-surface-observer.c \
	cairo-$(cairo_version)/src/cairo-surface-offset.c \
	cairo-$(cairo_version)/src/cairo-surface-snapshot.c \
	cairo-$(cairo_version)/src/cairo-surface-subsurface.c \
	cairo-$(cairo_version)/src/cairo-surface-wrapper.c \
	cairo-$(cairo_version)/src/cairo-surface.c \
	cairo-$(cairo_version)/src/cairo-tor-scan-converter.c \
	cairo-$(cairo_version)/src/cairo-tor22-scan-converter.c \
	cairo-$(cairo_version)/src/cairo-toy-font-face.c \
	cairo-$(cairo_version)/src/cairo-traps-compositor.c \
	cairo-$(cairo_version)/src/cairo-traps.c \
	cairo-$(cairo_version)/src/cairo-tristrip.c \
	cairo-$(cairo_version)/src/cairo-unicode.c \
	cairo-$(cairo_version)/src/cairo-user-font.c \
	cairo-$(cairo_version)/src/cairo-wideint.c \
	cairo-$(cairo_version)/src/cairo.c \
	cairo-$(cairo_version)/src/cairo-time.c \
	cairo-$(cairo_version)/src/cairo-ft-font.c \
	cairo-$(cairo_version)/src/cairo-png.c \
	# cairo-cairo_version.c \

pixman_sources = \
	pixman-$(pixman_version)/pixman/pixman.c \
	pixman-$(pixman_version)/pixman/pixman-access.c \
	pixman-$(pixman_version)/pixman/pixman-access-accessors.c \
	pixman-$(pixman_version)/pixman/pixman-bits-image.c \
	pixman-$(pixman_version)/pixman/pixman-combine32.c \
	pixman-$(pixman_version)/pixman/pixman-combine-float.c \
	pixman-$(pixman_version)/pixman/pixman-conical-gradient.c \
	pixman-$(pixman_version)/pixman/pixman-filter.c \
	pixman-$(pixman_version)/pixman/pixman-x86.c \
	pixman-$(pixman_version)/pixman/pixman-mips.c \
	pixman-$(pixman_version)/pixman/pixman-arm.c \
	pixman-$(pixman_version)/pixman/pixman-ppc.c \
	pixman-$(pixman_version)/pixman/pixman-edge.c \
	pixman-$(pixman_version)/pixman/pixman-edge-accessors.c \
	pixman-$(pixman_version)/pixman/pixman-fast-path.c \
	pixman-$(pixman_version)/pixman/pixman-glyph.c \
	pixman-$(pixman_version)/pixman/pixman-general.c \
	pixman-$(pixman_version)/pixman/pixman-gradient-walker.c \
	pixman-$(pixman_version)/pixman/pixman-image.c \
	pixman-$(pixman_version)/pixman/pixman-implementation.c \
	pixman-$(pixman_version)/pixman/pixman-linear-gradient.c \
	pixman-$(pixman_version)/pixman/pixman-matrix.c \
	pixman-$(pixman_version)/pixman/pixman-noop.c \
	pixman-$(pixman_version)/pixman/pixman-radial-gradient.c \
	pixman-$(pixman_version)/pixman/pixman-region16.c \
	pixman-$(pixman_version)/pixman/pixman-region32.c \
	pixman-$(pixman_version)/pixman/pixman-solid-fill.c \
	pixman-$(pixman_version)/pixman/pixman-timer.c \
	pixman-$(pixman_version)/pixman/pixman-trap.c \
	pixman-$(pixman_version)/pixman/pixman-utils.c \

libpng_sources = \
	libpng-$(libpng_version)/pngpread.o \
	libpng-$(libpng_version)/png.o \
	libpng-$(libpng_version)/pngerror.o \
	libpng-$(libpng_version)/pngmem.o \
	libpng-$(libpng_version)/pngget.o \
	libpng-$(libpng_version)/pngrtran.o \
	libpng-$(libpng_version)/pngwrite.o \
	libpng-$(libpng_version)/pngtrans.o \
	libpng-$(libpng_version)/pngwio.o \
	libpng-$(libpng_version)/pngset.o \
	libpng-$(libpng_version)/pngwtran.o \
	libpng-$(libpng_version)/pngrio.o \
	libpng-$(libpng_version)/pngread.o \
	libpng-$(libpng_version)/pngrutil.o \
	libpng-$(libpng_version)/pngwutil.o \

freetype_build_dir := $(BUILD_DIR)/libs/third_party/cairo/freetype-$(freetype_version)
include libs/third_party/cairo/freetype_objs.mk

objs-y += $(cairo_sources:.c=.o) $(pixman_sources:.c=.o) $(libpng_sources:.c=.o) $(freetype_objs)

cairo_all_sources := $(addprefix libs/third_party/cairo/, $(pixman_sources) $(cairo_sources) $(libpng_sources) $(fretype_sources))
cairo_all_objs := $(addprefix $(BUILD_DIR)/libs/third_party/cairo/, $(objs-y))

$(cairo_all_sources): $(cairo_dir) $(pixman_dir) $(libpng_dir) $(freetype_dir)
$(cairo_all_objs): CFLAGS += $(LIBC_CFLAGS) $(ZLIB_CFLAGS)
$(cairo_all_objs): SANITIZER :=

$(cairo_dir):
	$(PROGRESS) DOWNLOAD $(cairo_tarball_url)
	./tools/download-file.py --extract $(@D) $(cairo_tarball_url)

$(pixman_dir):
	$(PROGRESS) DOWNLOAD $(pixman_tarball_url)
	./tools/download-file.py --extract $(@D) $(pixman_tarball_url)

$(libpng_dir):
	$(PROGRESS) DOWNLOAD $(libpng_tarball_url)
	./tools/download-file.py --extract $(@D) $(libpng_tarball_url)

$(freetype_dir):
	$(PROGRESS) DOWNLOAD $(freetype_tarball_url)
	./tools/download-file.py --extract $(@D) $(freetype_tarball_url)
