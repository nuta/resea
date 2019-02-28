#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <types.h>
#include <ipc.h>

#define SYMBOL_TABLE_MAGIC 0x2b012b01
#define MSG_ID_SYMBOL_PREFIX 0xfaceface00000000ULL
#define BACKTRACE_MAX 16
#define STACK_CANARY 0xdeadca71 /* dead canary */

struct symbol_table_header {
    uint32_t magic;
    uint32_t num_symbols;
    uint32_t strbuf_offset;
    uint32_t strbuf_len;
} PACKED;

struct symbol {
    uint64_t addr;
    uint32_t strbuf_offset;
    uint32_t unused;
} PACKED;

struct symbol_table {
    struct symbol *symbols;
    size_t num_symbols;
    char *strbuf;
};

void backtrace(void);
const char *find_msg_name(payload_t msg_id);
void check_stack_canary(void);
void write_stack_canary(vaddr_t canary);
void init_boot_stack_canary(void);

void debug_init(void);

#endif
