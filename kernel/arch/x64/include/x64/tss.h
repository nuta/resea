#ifndef __X64_TSS_H__
#define __X64_TSS_H__

#include <types.h>

#define INTR_HANDLER_IST 1
#define EXP_HANDLER_IST  2

struct tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap;
} PACKED;

void x64_tss_init(void);

#endif
