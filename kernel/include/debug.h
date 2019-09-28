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

// TODO: Clean up.
enum asan_shadow_tag {
    ASAN_VALID         = 0xc1,
    ASAN_NOT_ALLOCATED = 0xe2,
    ASAN_UNINITIALIZED = 0x77,
};

void asan_init_area(enum asan_shadow_tag tag, void *ptr, size_t len);


void backtrace(void);
void check_stack_canary(void);
void init_stack_canary(vaddr_t stack_bottom);
void init_boot_stack_canary(void);
void debugger_oninterrupt(void);
void debug_init(void);

#endif
