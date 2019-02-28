#ifndef __X64_GDT_H__
#define __X64_GDT_H__

#include <types.h>

/* A TSS desc is twice as large as others. */
#define GDT_DESC_NUM 8

#define GDT_NULL         0
#define GDT_KERNEL_CODE  1
#define GDT_KERNEL_DATA  2
#define GDT_USER_CODE32  3
#define GDT_USER_DATA    4
#define GDT_USER_CODE64  5
#define GDT_TSS          6

#define KERNEL_CS (GDT_KERNEL_CODE * 8)
#define TSS_SEG   (GDT_TSS * 8)
#define USER_CS32 (GDT_USER_CODE32 * 8)
#define USER_CS64 (GDT_USER_CODE64 * 8)

/// Don't forget to update handler.S as well.
#define USER_DS   (GDT_USER_DATA * 8)
#define USER_RPL  3

struct gdtr {
    uint16_t len;
    uint64_t laddr;
} PACKED;

void x64_gdt_init(void);

#endif
