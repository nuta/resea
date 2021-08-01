# Since newlib's build system is a bit complicated for us, build it in a Docker
# container to make it easy to cross-compile.
FROM ubuntu:20.04
RUN \
    apt-get update \
    && apt-get install -qy build-essential curl

ARG NEWLIB_VERSION
RUN curl -fsSL https://sourceware.org/pub/newlib/newlib-$NEWLIB_VERSION.tar.gz | tar zxf - -C /tmp

WORKDIR /tmp/newlib-$NEWLIB_VERSION
RUN \
		mkdir -p build \
		&& cd build \
		&& ../newlib/configure \
            --target=x86_64-elf \
            --disable-multilib \
            --disable-shared \
            --disable-newlib-supplied-syscalls \
        && make -j8 CFLAGS+="-D_FORTIFY_SOURCE=0"
