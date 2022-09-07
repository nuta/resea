#pragma once
#include <types.h>

int memcmp(const void *p1, const void *p2, size_t len);
void *memset(void *dst, int ch, size_t len);
void *memcpy(void *dst, const void *src, size_t len);
void *memmove(void *dst, const void *src, size_t len);
size_t strlen(const char *s);
int strncmp(const char *s1, const char *s2, size_t len);
char *strncpy2(char *dst, const char *src, size_t len);
