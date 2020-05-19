#include <resea/rand.h>

void rand_bytes(uint8_t *buf, size_t len) {
#ifdef __x86_64__
    for (size_t i = 0; i < len; i += 4) {
        uint32_t r;
        uint64_t rflags;
        __asm__ __volatile__(
            "loop:             \n"
            "    rdrand %%eax  \n"
            "    jnc loop      \n"
            : "=a"(r), "=b"(rflags)
            :: "cc"
        );

        buf[i] = r & 0xff;
        buf[MIN(i + 1, len - 1)] = (r >> 8) & 0xff;
        buf[MIN(i + 2, len - 1)] = (r >> 16) & 0xff;
        buf[MIN(i + 3, len - 1)] = (r >> 24) & 0xff;
    }

#else
    // TODO:
#endif
}
