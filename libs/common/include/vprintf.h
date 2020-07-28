#ifndef __VPRINTF_H__
#define __VPRINTF_H__

#include <config.h>
#include <types.h>

#define SYMBOL_TABLE_EMPTY 0xb04d5953
#define SYMBOL_TABLE_MAGIC 0x4c4d5953
#define BACKTRACE_MAX      16

struct symbol {
    uint64_t addr;
    char name[56];
} __packed;

struct symbol_table {
    uint32_t magic;
    uint32_t num_symbols;
    uint64_t padding;
    struct symbol symbols[CONFIG_NUM_SYMBOLS];
} __packed;

struct stack_frame {
    struct stack_frame *next;
    uint64_t return_addr;
} __packed;

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
const char *msgtype2str(int type);

#endif
