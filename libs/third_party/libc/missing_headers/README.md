# Missing freestanding C standard library headers
Since newlib does not provide freestanding headers such as `stdarg.h`, as a
workaround, I added the ones make Cairo work on Resea.

Some of them are ported from [musl libc](https://www.musl-libc.org/).
