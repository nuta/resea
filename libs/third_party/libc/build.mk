name := libc
third-party-build := y
copyright := See https://sourceware.org/newlib/COPYING.NEWLIB

libc_version := 4.1.0
libc_tarball_url := https://sourceware.org/pub/newlib/newlib-$(libc_version).tar.gz

LIBC_SOUCE_DIRS = libs/third_party/libc/newlib-$(libc_version)

LIBC_CFLAGS += -D_XOPEN_SOURCE=700 -D_POSIX_C_SOURCE=200809 -D_DEFAULT_SOURCE -D_IEEE_LIBM
LIBC_CFLAGS += -Wno-sign-compare -Wno-unused-parameter

# FIXME:
LIBC_CFLAGS += -Wno-macro-redefined

LIBC_INCLUDES += -Ilibs/third_party/libc/newlib-$(libc_version)/newlib/libc/include
LIBC_INCLUDES += -Ilibs/third_party/libc/newlib-$(libc_version)/newlib/libm/common
LIBC_INCLUDES += -Ilibs/third_party/libc/missing_headers

$(build_dir)/libc.a:
	$(PROGRESS) "DOCKER" newlib

	$(DOCKER) build \
		-t resea_newlib_$(libc_version) \
		--build-arg NEWLIB_VERSION=$(libc_version) \
		- < libs/third_party/libc/Dockerfile
	mkdir -p $(@D)
	$(DOCKER) run resea_newlib_$(libc_version) cat /tmp/newlib-$(libc_version)/build/libc.a > $(@).tmp

	$(PROGRESS) "OBJCOPY" libc.a
	$(OBJCOPY) \
		--redefine-syms=libs/third_party/libc/symbol_overrides.txt \
		$(@).tmp $@

$(build_dir)/libm.a: $(build_dir)/libc.a
	mkdir -p $(@D)
	$(DOCKER) run resea_newlib_$(libc_version) cat /tmp/newlib-$(libc_version)/build/libm.a > $(@).tmp

	$(PROGRESS) "OBJCOPY" libm.a
	$(OBJCOPY) \
		--redefine-syms=libs/third_party/libc/symbol_overrides.txt \
		$(@).tmp $@

# Let objects depend on source directories neeeded to be downloaded by curl.
$(addprefix $(build_dir)/, $(LIBC_ARCHIVES)): $(LIBC_SOUCE_DIRS)
$(build_dir)/newlib_syscalls.o: $(LIBC_SOUCE_DIRS)

libs/third_party/libc/newlib-$(libc_version):
	$(PROGRESS) CURL $(libc_tarball_url)
	curl -fsSL $(libc_tarball_url) | tar zxf - -C libs/third_party/libc

cflags-y += $(LIBC_CFLAGS) $(LIBC_INCLUDES)

objs-y += newlib_syscalls.o
LIBC_ARCHIVES += libc.a libm.a
