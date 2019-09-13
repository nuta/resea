#ifndef __RESEA_STRING_H__
#define __RESEA_STRING_H__

#include <types.h>

int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t len);
int memcmp(const void *s1, const void *s2, size_t len);
void *memset(void *dst, int ch, size_t len);
// TODO: Use __builtin_memcpy. Perhaps it optimizes a constant-sized small copy.
void *memcpy(void *dst, const void *src, size_t len);
void *memcpy_s(void *dst, size_t dst_len, const void *src, size_t copy_len);

#endif
