#ifndef __RESEA_BACKTRACE_H__
#define __RESEA_BACKTRACE_H__

#include <resea.h>

struct symbol {
    uint64_t addr;
    uint16_t offset;
    uint8_t reserved[6];
} PACKED;

struct symbol_table {
    uint32_t magic;
    uint16_t num_symbols;
    uint16_t reserved;
    struct symbol *symbols;
    char *strbuf;
} PACKED;

#endif
