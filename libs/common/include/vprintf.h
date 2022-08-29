#pragma once
#include <types.h>

struct vprintf_context {
    void (*printchar)(struct vprintf_context *ctx, char ch);
    char *buf;
    size_t size;
    size_t index;
};

void vprintf_with_context(struct vprintf_context *ctx, const char *fmt,
                          va_list vargs);
void printf_with_context(struct vprintf_context *ctx, const char *fmt, ...);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list vargs);
int snprintf(char *buf, size_t size, const char *fmt, ...);
