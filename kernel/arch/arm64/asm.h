#ifndef __ARM64_ASM_H__
#define __ARM64_ASM_H__

#include <types.h>

#define ARM64_MSR(reg, value) \
    __asm__ __volatile__("msr " #reg ", %0" :: "r"(value))

#define ARM64_MRS(reg) \
    ({ uint64_t value; \
        __asm__ __volatile__("mrs %0, " #reg : "=r"(value)); \
        value; })

#endif
