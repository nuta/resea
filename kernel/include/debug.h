#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <types.h>

#define SYMBOL_TABLE_MAGIC 0x2b012b01
#define BACKTRACE_MAX 16
#define STACK_CANARY 0xdeadca71 /* dead canary */

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

void backtrace(void);
void check_stack_canary(void);
void init_stack_canary(vaddr_t stack_bottom);
void init_boot_stack_canary(void);
void debug_init(void);

#endif
