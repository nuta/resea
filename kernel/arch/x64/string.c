#include <arch.h>
#include <printk.h>

void *memset(void *dst, size_t dst_len, char ch, size_t len) {
    if (dst_len < len) {
        PANIC("bad memset");
    }

    __asm__ __volatile__("cld; rep stosb" :: "D"(dst), "a"(ch), "c"(len));
    return dst;
}

void *memcpy(void *dst, size_t dst_len, void *src, size_t copy_len) {
    if (dst_len < copy_len) {
        PANIC("bad memcpy");
    }

    __asm__ __volatile__("cld; rep movsb" :: "D"(dst), "S"(src), "c"(copy_len));
    return dst;
}

char *strcpy(char *dst, size_t dst_len, const char *src) {
    if (dst_len == 0) {
        PANIC("bad strcpy");
    }

    size_t i = 0;
    while (i < dst_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
    return dst;
}
