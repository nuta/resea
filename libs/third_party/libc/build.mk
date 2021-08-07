name := libc
third-party-build := y
copyright := See https://sourceware.org/newlib/COPYING.NEWLIB
libc_version := 4.1.0
libc_tarball_url := https://sourceware.org/pub/newlib/newlib-$(libc_version).tar.gz

newlib_dir := libs/third_party/libc/newlib-$(libc_version)
targ_include_dir := $(BUILD_DIR)/libs/libc/targ-include

# Cflags for library users.
LIBC_CFLAGS += -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809 -D_IEEE_LIBM
LIBC_CFLAGS += -Wno-sign-compare -Wno-unused-parameter -Wno-incompatible-library-redeclaration
LIBC_CFLAGS += -Ilibs/third_party/libc/newlib-$(libc_version)/newlib/libc/include
LIBC_CFLAGS += -Ilibs/third_party/libc/newlib-$(libc_version)/newlib/libm/common
LIBC_CFLAGS += -Ilibs/third_party/libc/missing_headers

objs-y += newlib_all.o newlib_syscalls.o
cflags-y += $(LIBC_CFLAGS)
autogen-files-y += $(newlib_dir) $(targ_include_dir)

# TODO: Support architectures in addition to x86-64

#
#  Internal build rules
#

$(newlib_dir):
	$(PROGRESS) DOWNLOAD $(libc_tarball_url)
	./tools/download-file.py --extract $(@D) $(libc_tarball_url)

$(targ_include_dir): $(newlib_dir)
	$(PROGRESS) GEN $@
	mkdir -p $(@).tmp/sys
	mkdir -p $(@).tmp/machine
	mkdir -p $(@).tmp/bits

	for i in $(newlib_dir)/newlib/libc/machine/x86_64/machine/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/machine/`basename $$i`; \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/machine/x86_64/sys/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/sys/`basename $$i`; \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/machine/x86_64/include/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/`basename $$i`; \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/sys/include/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/`basename $$i`; \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/sys/include/*; do \
	    if [ -d $$i ]; then \
		for j in $$i/*.h; do \
		    if [ ! -d $(@).tmp/`basename $$i` ]; then \
		    	mkdir $(@).tmp/`basename $$i`; \
		    fi; \
	      	    cp $j $(@).tmp/`basename $$i`/`basename $j`; \
		done \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/sys/sys/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/sys/`basename $$i`; \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/sys/bits/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/bits/`basename $$i`; \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/sys/machine/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/machine/`basename $$i`; \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/sys/machine/x86_64/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/machine/`basename $$i`; \
	    fi ; \
	done
	for i in $(newlib_dir)/newlib/libc/sys/machine/x86_64/include/*.h; do \
	    if [ -f $$i ]; then \
	      cp $$i $(@).tmp/machine/`basename $$i`; \
	    fi ; \
	done

	mv $(@).tmp $(@)

# Let objects depend on source directories neeeded to be downloaded by curl.
$(addprefix $(build_dir)/, $(LIBC_ARCHIVES)): $(newlib_dir)
$(build_dir)/newlib_syscalls.o: $(newlib_dir)

newlib_common_cflags += -Wno-logical-not-parentheses -Wno-unknown-pragmas -Wno-pointer-sign
newlib_common_cflags += -Wno-unknown-attributes -Wno-array-bounds -Wno-implicit-function-declaration
newlib_common_cflags += -Wno-shift-negative-value -Wno-empty-body -Wno-int-conversion
newlib_common_cflags += -Ilibs/third_party/libc/internal_headers
newlib_common_cflags += -fno-stack-protector -fno-builtin -nostdinc
newlib_common_cflags += -D__NO_SYSCALLS__ -DMISSING_SYSCALL_NAMES -DHAVE_INIT_FINI
newlib_common_cflags += -D_FORTIFY_SOURCE=0 -D_DEFAULT_SOURCE
newlib_common_cflags += -I$(newlib_dir)

# Defines $(newlib_objs).
newlib_build_dir := $(BUILD_DIR)/libs/third_party/libc
include libs/third_party/libc/newlib_objs.mk
include libs/third_party/libc/compile_rules.mk

$(newlib_build_dir)/newlib_all.o: $(addprefix $(newlib_build_dir)/, $(newlib_objs))
	$(PROGRESS) "LD" $(@)
	$(LD) -r -o $(@).tmp $(addprefix $(newlib_build_dir)/, $(newlib_objs))

	$(PROGRESS) "OBJCOPY" $(@)
	$(OBJCOPY) \
		--redefine-syms=libs/third_party/libc/symbol_overrides.txt \
		$(@).tmp $@

$(addprefix $(newlib_build_dir)/, $(newlib_objs)): $(targ_include_dir)
	$(PROGRESS) "CC" $(srcfile)
	mkdir -p $(@D)
	$(CC) -I$(targ_include_dir) $(perfile_cflags) $(newlib_common_cflags) \
		$(CFLAGS) -U_XOPEN_SOURCE \
		-c -o $@ $(srcfile) -MD -MF $(@:.o=.deps) -MJ $(@:.o=.json)

