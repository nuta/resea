#ifndef __VPRINTF_H__
#define __VPRINTF_H__

#include <types.h>

#define SYMBOL_TABLE_EMPTY 0xb04d5953b04d5953
#define SYMBOL_TABLE_MAGIC 0x4c4d59534c4d5953
#define BACKTRACE_MAX      16
#define SYMBOLS_MAX        256

struct symbol {
    uint64_t addr;
    char name[56];
} PACKED;

struct symbol_table {
    uint64_t magic;
    uint32_t num_symbols;
    uint32_t padding;
    struct symbol symbols[SYMBOLS_MAX];
} PACKED;

struct stack_frame {
    struct stack_frame *next;
    uint64_t return_addr;
} PACKED;

struct vprintf_context {
    void (*printchar)(struct vprintf_context *ctx, char ch);
    char *buf;
    size_t size;
    size_t index;
};

void vprintf(struct vprintf_context *ctx, const char *fmt, va_list vargs);
int vsnprintf(char *buf, size_t size, const char *fmt, va_list vargs);
int snprintf(char *buf, size_t size, const char *fmt, ...);
void backtrace(void);
const char *err2str(error_t err);

#endif
