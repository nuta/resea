#ifndef __BACKTRACE_H__
#define __BACKTRACE_H__

#include <types.h>

#define BACKTRACE_MAX 16
#define SYMBOL_TABLE_MAGIC 0x2b012b01

struct symbol {
    uint64_t addr;
    uint32_t offset;
    uint32_t name_len;
} PACKED;

struct symbol_table {
    uint32_t magic;
    uint32_t num_symbols;
    struct symbol *symbols;
    char *strbuf;
    uint32_t strbuf_len;
} PACKED;

void backtrace(void);

#endif
