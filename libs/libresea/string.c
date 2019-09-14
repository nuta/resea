#include <resea.h>

size_t strlen(const char *s) {
    size_t len = 0;
    while (*s != '\0') {
        len++;
        s++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *s1 - *s2;
}

int strncmp(const char *s1, const char *s2, size_t len) {
    while (len > 0 && *s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
        len--;
    }

    return (len == 0) ? 0 : (*s1 - *s2);
}

int memcmp(const void *s1, const void *s2, size_t len) {
    uint8_t *p = (uint8_t *) s1;
    uint8_t *q = (uint8_t *) s2;
    while (len > 0 && *p == *q) {
        p++;
        q++;
        len--;
    }

    return (len == 0) ? 0 : (*p - *q);
}

void *memset(void *dst, int ch, size_t len) {
    assert(dst != NULL && "copy to NULL");
    for (size_t i = 0; i < len; i++) {
        *((uint8_t *) dst + i) =  ch;
    }
    return dst;
}

void *memcpy(void *dst, const void *src, size_t len) {
    assert(dst != NULL && "copy to NULL");
    assert(src != NULL && "copy from NULL");
    for (size_t i = 0; i < len; i++) {
        *((uint8_t *) dst + i) = *((uint8_t *) src + i);
    }
    return dst;
}

void *memcpy_s(void *dst, size_t dst_len, const void *src, size_t copy_len) {
    assert(copy_len <= dst_len && "bad memcpy");
    return memcpy(dst, src, copy_len);
}
