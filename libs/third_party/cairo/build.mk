name := cairo
third-party-build := y
copyright := See https://github.com/freedesktop/cairo/blob/master/COPYING and https://github.com/freedesktop/pixman/blob/master/COPYING

cairo_version := 1.17.4
pixman_version := 0.40.0
cairo_tarball_url := https://cairographics.org/snapshots/cairo-$(cairo_version).tar.xz
pixman_tarball_url := https://cairographics.org/releases/pixman-$(pixman_version).tar.gz

CAIRO_SOUCE_DIRS = \
	libs/third_party/cairo/cairo-$(cairo_version) \
	libs/third_party/cairo/pixman-$(pixman_version)

CAIRO_CFLAGS += -Wno-return-type -Wno-missing-braces -Wno-enum-conversion
CAIRO_CFLAGS += -Wno-missing-field-initializers -Wno-unused
CAIRO_CFLAGS += -Wno-implicit-function-declaration
CAIRO_CFLAGS += -Wno-incompatible-pointer-types
CAIRO_CFLAGS += -Wno-parentheses-equality
CAIRO_CFLAGS += -DHAVE_STDINT_H -DHAVE_UINT64_T -DHAVE_INTTYPES_H
CAIRO_CFLAGS += -DHAVE_STDLIB_H -DHAVE_STRINGS_H -DHAVE_STRING_H -DHAVE_MEMORY_H
CAIRO_CFLAGS += -DPIXMAN_NO_TLS -DPACKAGE

CAIRO_INCLUDES += -Ilibs/third_party/cairo/internal_headers
CAIRO_INCLUDES += -Ilibs/third_party/cairo/cairo-$(cairo_version)/src
CAIRO_INCLUDES += -Ilibs/third_party/cairo/pixman-$(pixman_version)/pixman

cairo_sources = \
	cairo-analysis-surface.c \
	cairo-arc.c \
	cairo-array.c \
	cairo-atomic.c \
	cairo-base64-stream.c \
	cairo-base85-stream.c \
	cairo-bentley-ottmann-rectangular.c \
	cairo-bentley-ottmann-rectilinear.c \
	cairo-bentley-ottmann.c \
	cairo-botor-scan-converter.c \
	cairo-boxes-intersect.c \
	cairo-boxes.c \
	cairo-cache.c \
	cairo-clip-boxes.c \
	cairo-clip-polygon.c \
	cairo-clip-region.c \
	cairo-clip-surface.c \
	cairo-clip-tor-scan-converter.c \
	cairo-clip.c \
	cairo-color.c \
	cairo-composite-rectangles.c \
	cairo-compositor.c \
	cairo-contour.c \
	cairo-damage.c \
	cairo-debug.c \
	cairo-default-context.c \
	cairo-device.c \
	cairo-error.c \
	cairo-fallback-compositor.c \
	cairo-fixed.c \
	cairo-font-face-twin-data.c \
	cairo-font-face-twin.c \
	cairo-font-face.c \
	cairo-font-options.c \
	cairo-freed-pool.c \
	cairo-freelist.c \
	cairo-gstate.c \
	cairo-hash.c \
	cairo-hull.c \
	cairo-image-compositor.c \
	cairo-image-info.c \
	cairo-image-source.c \
	cairo-image-surface.c \
	cairo-line.c \
	cairo-lzw.c \
	cairo-mask-compositor.c \
	cairo-matrix.c \
	cairo-mempool.c \
	cairo-mesh-pattern-rasterizer.c \
	cairo-misc.c \
	cairo-mono-scan-converter.c \
	cairo-mutex.c \
	cairo-no-compositor.c \
	cairo-observer.c \
	cairo-output-stream.c \
	cairo-paginated-surface.c \
	cairo-path-bounds.c \
	cairo-path-fill.c \
	cairo-path-fixed.c \
	cairo-path-in-fill.c \
	cairo-path-stroke-boxes.c \
	cairo-path-stroke-polygon.c \
	cairo-path-stroke-traps.c \
	cairo-path-stroke-tristrip.c \
	cairo-path-stroke.c \
	cairo-path.c \
	cairo-pattern.c \
	cairo-pen.c \
	cairo-polygon-intersect.c \
	cairo-polygon-reduce.c \
	cairo-polygon.c \
	cairo-raster-source-pattern.c \
	cairo-recording-surface.c \
	cairo-rectangle.c \
	cairo-rectangular-scan-converter.c \
	cairo-region.c \
	cairo-rtree.c \
	cairo-scaled-font.c \
	cairo-shape-mask-compositor.c \
	cairo-slope.c \
	cairo-spans-compositor.c \
	cairo-spans.c \
	cairo-spline.c \
	cairo-stroke-dash.c \
	cairo-stroke-style.c \
	cairo-surface-clipper.c \
	cairo-surface-fallback.c \
	cairo-surface-observer.c \
	cairo-surface-offset.c \
	cairo-surface-snapshot.c \
	cairo-surface-subsurface.c \
	cairo-surface-wrapper.c \
	cairo-surface.c \
	cairo-tor-scan-converter.c \
	cairo-tor22-scan-converter.c \
	cairo-toy-font-face.c \
	cairo-traps-compositor.c \
	cairo-traps.c \
	cairo-tristrip.c \
	cairo-unicode.c \
	cairo-user-font.c \
	cairo-wideint.c \
	cairo.c \
	cairo-time.c \
	# cairo-cairo_version.c \

pixman_sources =			\
	pixman.c			\
	pixman-access.c			\
	pixman-access-accessors.c	\
	pixman-bits-image.c		\
	pixman-combine32.c		\
	pixman-combine-float.c		\
	pixman-conical-gradient.c	\
	pixman-filter.c			\
	pixman-x86.c			\
	pixman-mips.c			\
	pixman-arm.c			\
	pixman-ppc.c			\
	pixman-edge.c			\
	pixman-edge-accessors.c		\
	pixman-fast-path.c		\
	pixman-glyph.c			\
	pixman-general.c		\
	pixman-gradient-walker.c	\
	pixman-image.c			\
	pixman-implementation.c		\
	pixman-linear-gradient.c	\
	pixman-matrix.c			\
	pixman-noop.c			\
	pixman-radial-gradient.c	\
	pixman-region16.c		\
	pixman-region32.c		\
	pixman-solid-fill.c		\
	pixman-timer.c			\
	pixman-trap.c			\
	pixman-utils.c			\

# cairo.c -> cairo-X.X.X/src/cairo.o
cairo_objs = $(addprefix cairo-$(cairo_version)/src/, $(patsubst %.c, %.o, $(cairo_sources)))

# pixman.c -> pixman-X.X.X/pixman.o
pixman_objs = $(addprefix pixman-$(pixman_version)/pixman/, $(patsubst %.c, %.o, $(pixman_sources)))

# Let objects depend on source directories neeeded to be downloaded by curl.
$(addprefix $(BUILD_DIR)/libs/third_party/cairo/, $(cairo_objs)):  $(LIBC_SOUCE_DIRS) $(CAIRO_SOUCE_DIRS)
$(addprefix $(BUILD_DIR)/libs/third_party/cairo/, $(pixman_objs)): $(LIBC_SOUCE_DIRS) $(CAIRO_SOUCE_DIRS)
$(addprefix $(BUILD_DIR)/libs/third_party/cairo/, $(cairo_objs)):  CFLAGS += $(LIBC_CFLAGS) $(LIBC_INCLUDES)
$(addprefix $(BUILD_DIR)/libs/third_party/cairo/, $(pixman_objs)): CFLAGS += $(LIBC_CFLAGS) $(LIBC_INCLUDES)

libs/third_party/cairo/cairo-$(cairo_version):
	$(PROGRESS) CURL $(cairo_tarball_url)
	curl -fsSL $(cairo_tarball_url) | tar Jxf - -C libs/third_party/cairo

libs/third_party/cairo/pixman-$(pixman_version):
	$(PROGRESS) CURL $(pixman_tarball_url)
	curl -fsSL $(pixman_tarball_url) | tar zxf - -C libs/third_party/cairo

objs-y += $(cairo_objs) $(pixman_objs)
cflags-y += $(CAIRO_CFLAGS) $(CAIRO_INCLUDES)
