#include <arch.h>

WEAK char *strcpy(char *dst, size_t dst_len, const char *src) {
    DEBUG_ASSERT(dst != NULL && "copy to NULL");
    DEBUG_ASSERT(src != NULL && "copy from NULL");

    size_t i = 0;
    while (i < dst_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
    return dst;
}

WEAK int strcmp(const char *s1, const char *s2) {
    DEBUG_ASSERT(s1 != NULL && "s1 NULL");
    DEBUG_ASSERT(s2 != NULL && "s2 NULL");

    while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}
