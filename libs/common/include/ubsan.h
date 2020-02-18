#ifndef __UBSAN_H__
#define __UBSAN_H__

#include <types.h>

struct ubsan_type {
    uint16_t kind;
    uint16_t info;
    char name[];
};

struct ubsan_sourceloc {
    const char *file;
    uint32_t line;
    uint32_t column;
};

struct ubsan_mismatch_data_v1 {
    struct ubsan_sourceloc loc;
    struct ubsan_type *type;
    uint8_t align;
    uint8_t kind;
};

#endif
