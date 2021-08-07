freetype_cflags += -Wall -g -O2 -fvisibility=hidden -pthread
freetype_cflags += -mno-sse2
freetype_cflags += "-DFT_CONFIG_CONFIG_H=<ftconfig.h>" "-DFT_CONFIG_MODULES_H=<ftmodule.h>" "-DFT_CONFIG_OPTIONS_H=<ftoption.h>" -DFT2_BUILD_LIBRARY
freetype_cflags += -I$(freetype_dir)/include -Ilibs/third_party/cairo/internal_headers
freetype_cflags += -I$(freetype_dir)/include/freetype/config

$(freetype_build_dir)/ftsystem.o:   perfile_cflags += -I$(freetype_dir)/objs
$(freetype_build_dir)/ftsystem.o:   srcfile        := $(freetype_dir)/builds/unix/ftsystem.c
$(freetype_build_dir)/ftcid.o:      perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftcid.o:      srcfile        := $(freetype_dir)/src/base/ftcid.c
$(freetype_build_dir)/ftdebug.o:    perfile_cflags += -I$(freetype_dir)/objs
$(freetype_build_dir)/ftdebug.o:    srcfile        := $(freetype_dir)/src/base/ftdebug.c
$(freetype_build_dir)/ftbbox.o:     perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftbbox.o:     srcfile        := $(freetype_dir)/src/base/ftbbox.c
$(freetype_build_dir)/ftinit.o:     perfile_cflags += -I$(freetype_dir)/objs
$(freetype_build_dir)/ftinit.o:     srcfile        := $(freetype_dir)/src/base/ftinit.c
$(freetype_build_dir)/ftmm.o:       perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftmm.o:       srcfile        := $(freetype_dir)/src/base/ftmm.c
$(freetype_build_dir)/ftgasp.o:     perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftgasp.o:     srcfile        := $(freetype_dir)/src/base/ftgasp.c
$(freetype_build_dir)/ftbase.o:     perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftbase.o:     srcfile        := $(freetype_dir)/src/base/ftbase.c
$(freetype_build_dir)/ftbitmap.o:   perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftbitmap.o:   srcfile        := $(freetype_dir)/src/base/ftbitmap.c
$(freetype_build_dir)/ftotval.o:    perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftotval.o:    srcfile        := $(freetype_dir)/src/base/ftotval.c
$(freetype_build_dir)/ftbdf.o:      perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftbdf.o:      srcfile        := $(freetype_dir)/src/base/ftbdf.c
$(freetype_build_dir)/fttype1.o:    perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/fttype1.o:    srcfile        := $(freetype_dir)/src/base/fttype1.c
$(freetype_build_dir)/ftsynth.o:    perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftsynth.o:    srcfile        := $(freetype_dir)/src/base/ftsynth.c
$(freetype_build_dir)/ftfstype.o:   perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftfstype.o:   srcfile        := $(freetype_dir)/src/base/ftfstype.c
$(freetype_build_dir)/ftpatent.o:   perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftpatent.o:   srcfile        := $(freetype_dir)/src/base/ftpatent.c
$(freetype_build_dir)/ftpfr.o:      perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftpfr.o:      srcfile        := $(freetype_dir)/src/base/ftpfr.c
$(freetype_build_dir)/ftgxval.o:    perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftgxval.o:    srcfile        := $(freetype_dir)/src/base/ftgxval.c
$(freetype_build_dir)/type1.o:      perfile_cflags += -I$(freetype_dir)/src/type1
$(freetype_build_dir)/type1.o:      srcfile        := $(freetype_dir)/src/type1/type1.c
$(freetype_build_dir)/truetype.o:   perfile_cflags += -I$(freetype_dir)/src/truetype
$(freetype_build_dir)/truetype.o:   srcfile        := $(freetype_dir)/src/truetype/truetype.c
$(freetype_build_dir)/ftglyph.o:    perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftglyph.o:    srcfile        := $(freetype_dir)/src/base/ftglyph.c
$(freetype_build_dir)/type1cid.o:   perfile_cflags += -I$(freetype_dir)/src/cid
$(freetype_build_dir)/type1cid.o:   srcfile        := $(freetype_dir)/src/cid/type1cid.c
$(freetype_build_dir)/ftwinfnt.o:   perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftwinfnt.o:   srcfile        := $(freetype_dir)/src/base/ftwinfnt.c
$(freetype_build_dir)/ftstroke.o:   perfile_cflags += -I$(freetype_dir)/src/base
$(freetype_build_dir)/ftstroke.o:   srcfile        := $(freetype_dir)/src/base/ftstroke.c
$(freetype_build_dir)/cff.o:        perfile_cflags += -I$(freetype_dir)/src/cff
$(freetype_build_dir)/cff.o:        srcfile        := $(freetype_dir)/src/cff/cff.c
$(freetype_build_dir)/type42.o:     perfile_cflags += -I$(freetype_dir)/src/type42
$(freetype_build_dir)/type42.o:     srcfile        := $(freetype_dir)/src/type42/type42.c
$(freetype_build_dir)/pfr.o:        perfile_cflags += -I$(freetype_dir)/src/pfr
$(freetype_build_dir)/pfr.o:        srcfile        := $(freetype_dir)/src/pfr/pfr.c
$(freetype_build_dir)/pcf.o:        perfile_cflags += -I$(freetype_dir)/src/pcf
$(freetype_build_dir)/pcf.o:        srcfile        := $(freetype_dir)/src/pcf/pcf.c
$(freetype_build_dir)/winfnt.o:     perfile_cflags += -I$(freetype_dir)/src/winfonts
$(freetype_build_dir)/winfnt.o:     srcfile        := $(freetype_dir)/src/winfonts/winfnt.c
$(freetype_build_dir)/pshinter.o:   perfile_cflags += -I$(freetype_dir)/src/pshinter
$(freetype_build_dir)/pshinter.o:   srcfile        := $(freetype_dir)/src/pshinter/pshinter.c
$(freetype_build_dir)/bdf.o:        perfile_cflags += -I$(freetype_dir)/src/bdf
$(freetype_build_dir)/bdf.o:        srcfile        := $(freetype_dir)/src/bdf/bdf.c
$(freetype_build_dir)/raster.o:     perfile_cflags += -I$(freetype_dir)/src/raster
$(freetype_build_dir)/raster.o:     srcfile        := $(freetype_dir)/src/raster/raster.c
$(freetype_build_dir)/sfnt.o:       perfile_cflags += -I$(freetype_dir)/src/sfnt
$(freetype_build_dir)/sfnt.o:       srcfile        := $(freetype_dir)/src/sfnt/sfnt.c
$(freetype_build_dir)/smooth.o:     perfile_cflags += -I$(freetype_dir)/src/smooth
$(freetype_build_dir)/smooth.o:     srcfile        := $(freetype_dir)/src/smooth/smooth.c
$(freetype_build_dir)/autofit.o:    perfile_cflags += -I$(freetype_dir)/src/autofit
$(freetype_build_dir)/autofit.o:    srcfile        := $(freetype_dir)/src/autofit/autofit.c
$(freetype_build_dir)/ftcache.o:    perfile_cflags += -I$(freetype_dir)/src/cache
$(freetype_build_dir)/ftcache.o:    srcfile        := $(freetype_dir)/src/cache/ftcache.c
$(freetype_build_dir)/sdf.o:        perfile_cflags += -I$(freetype_dir)/src/sdf
$(freetype_build_dir)/sdf.o:        srcfile        := $(freetype_dir)/src/sdf/sdf.c
$(freetype_build_dir)/ftgzip.o:     perfile_cflags += -I$(freetype_dir)/objs
$(freetype_build_dir)/ftgzip.o:     srcfile        := $(freetype_dir)/src/gzip/ftgzip.c
$(freetype_build_dir)/ftlzw.o:      perfile_cflags += -I$(freetype_dir)/src/lzw
$(freetype_build_dir)/ftlzw.o:      srcfile        := $(freetype_dir)/src/lzw/ftlzw.c
$(freetype_build_dir)/ftbzip2.o:    perfile_cflags += -I$(freetype_dir)/objs
$(freetype_build_dir)/ftbzip2.o:    srcfile        := $(freetype_dir)/src/bzip2/ftbzip2.c
$(freetype_build_dir)/psaux.o:      perfile_cflags += -I$(freetype_dir)/src/psaux
$(freetype_build_dir)/psaux.o:      srcfile        := $(freetype_dir)/src/psaux/psaux.c
$(freetype_build_dir)/psnames.o:    perfile_cflags += -I$(freetype_dir)/src/psnames
$(freetype_build_dir)/psnames.o:    srcfile        := $(freetype_dir)/src/psnames/psnames.c
$(freetype_build_dir)/dlg.o:        perfile_cflags += -I$(freetype_dir)/src/dlg
$(freetype_build_dir)/dlg.o:        srcfile        := $(freetype_dir)/src/dlg/dlgwrap.c

freetype_sources := \
        builds/unix/ftsystem.c \
        $(freetype_dir)/src/base/ftcid.c \
        $(freetype_dir)/src/base/ftdebug.c \
        $(freetype_dir)/src/base/ftbbox.c \
        $(freetype_dir)/src/base/ftinit.c \
        $(freetype_dir)/src/base/ftmm.c \
        $(freetype_dir)/src/base/ftgasp.c \
        $(freetype_dir)/src/base/ftbase.c \
        $(freetype_dir)/src/base/ftbitmap.c \
        $(freetype_dir)/src/base/ftotval.c \
        $(freetype_dir)/src/base/ftbdf.c \
        $(freetype_dir)/src/base/fttype1.c \
        $(freetype_dir)/src/base/ftsynth.c \
        $(freetype_dir)/src/base/ftfstype.c \
        $(freetype_dir)/src/base/ftpatent.c \
        $(freetype_dir)/src/base/ftpfr.c \
        $(freetype_dir)/src/base/ftgxval.c \
        $(freetype_dir)/src/type1/type1.c \
        $(freetype_dir)/src/truetype/truetype.c \
        $(freetype_dir)/src/base/ftglyph.c \
        $(freetype_dir)/src/cid/type1cid.c \
        $(freetype_dir)/src/base/ftwinfnt.c \
        $(freetype_dir)/src/base/ftstroke.c \
        $(freetype_dir)/src/cff/cff.c \
        $(freetype_dir)/src/type42/type42.c \
        $(freetype_dir)/src/pfr/pfr.c \
        $(freetype_dir)/src/pcf/pcf.c \
        $(freetype_dir)/src/winfonts/winfnt.c \
        $(freetype_dir)/src/pshinter/pshinter.c \
        $(freetype_dir)/src/bdf/bdf.c \
        $(freetype_dir)/src/raster/raster.c \
        $(freetype_dir)/src/sfnt/sfnt.c \
        $(freetype_dir)/src/smooth/smooth.c \
        $(freetype_dir)/src/autofit/autofit.c \
        $(freetype_dir)/src/cache/ftcache.c \
        $(freetype_dir)/src/sdf/sdf.c \
        $(freetype_dir)/src/lzw/ftlzw.c \
        $(freetype_dir)/src/psaux/psaux.c \
        $(freetype_dir)/src/psnames/psnames.c \
        $(freetype_dir)/src/dlg/dlgwrap.c \

freetype_objs := \
        freetype-$(freetype_version)/ftcid.o \
        freetype-$(freetype_version)/ftdebug.o \
        freetype-$(freetype_version)/ftbbox.o \
        freetype-$(freetype_version)/ftinit.o \
        freetype-$(freetype_version)/ftmm.o \
        freetype-$(freetype_version)/ftgasp.o \
        freetype-$(freetype_version)/ftbase.o \
        freetype-$(freetype_version)/ftbitmap.o \
        freetype-$(freetype_version)/ftotval.o \
        freetype-$(freetype_version)/ftbdf.o \
        freetype-$(freetype_version)/fttype1.o \
        freetype-$(freetype_version)/ftsynth.o \
        freetype-$(freetype_version)/ftfstype.o \
        freetype-$(freetype_version)/ftpatent.o \
        freetype-$(freetype_version)/ftpfr.o \
        freetype-$(freetype_version)/ftgxval.o \
        freetype-$(freetype_version)/type1.o \
        freetype-$(freetype_version)/truetype.o \
        freetype-$(freetype_version)/ftglyph.o \
        freetype-$(freetype_version)/type1cid.o \
        freetype-$(freetype_version)/ftwinfnt.o \
        freetype-$(freetype_version)/ftstroke.o \
        freetype-$(freetype_version)/cff.o \
        freetype-$(freetype_version)/type42.o \
        freetype-$(freetype_version)/pfr.o \
        freetype-$(freetype_version)/pcf.o \
        freetype-$(freetype_version)/winfnt.o \
        freetype-$(freetype_version)/pshinter.o \
        freetype-$(freetype_version)/bdf.o \
        freetype-$(freetype_version)/raster.o \
        freetype-$(freetype_version)/sfnt.o \
        freetype-$(freetype_version)/smooth.o \
        freetype-$(freetype_version)/autofit.o \
        freetype-$(freetype_version)/ftcache.o \
        freetype-$(freetype_version)/sdf.o \
        freetype-$(freetype_version)/ftgzip.o \
        freetype-$(freetype_version)/ftlzw.o \
        freetype-$(freetype_version)/ftbzip2.o \
        freetype-$(freetype_version)/psaux.o \
        freetype-$(freetype_version)/psnames.o \
        freetype-$(freetype_version)/dlg.o \
        freetype-$(freetype_version)/ftsystem.o \

$(addprefix $(BUILD_DIR)/libs/third_party/cairo/, $(freetype_objs)):
	$(PROGRESS) "CC" $(srcfile)
	mkdir -p $(@D)
	$(CC) $(perfile_cflags) $(freetype_cflags) \
		$(CFLAGS) $(LIBC_CFLAGS) \
		-c -o $@ $(srcfile) -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

