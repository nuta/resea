#ifndef __X64_IDT_H__
#define __X64_IDT_H__

#include <types.h>

#define IDT_DESC_NUM 256
#define IDT_INT_HANDLER 0x8e

struct idt_entry {
    uint16_t offset1;
    uint16_t seg;
    uint8_t  ist;
    uint8_t  info;
    uint16_t offset2;
    uint32_t offset3;
    uint32_t reserved;
} PACKED;

struct idtr {
    uint16_t len;
    uint64_t laddr;
} PACKED;

void x64_idt_init(void);

#endif
