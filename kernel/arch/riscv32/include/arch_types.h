#pragma once
#include <types.h>

struct arch_task {
    uint32_t sp;
    uint32_t sp_top;
};

struct arch_vm {
    paddr_t table;
};

struct arch_cpuvar {
    // TODO: asmoffset
    // 4 * 0 = 0
    uint32_t sscratch;
    // 4 * 1 = 4
    uint32_t sp;
    // 4 * 2 = 8
    uint32_t mscratch[2];
    // 4 * 4 = 16
    uint32_t mtimecmp;
    // 4 * 5 = 20
    uint32_t mtime;
    // 4 * 6 = 24
    uint32_t interval;
    // 4 * 7 = 28
    uint32_t sscratch2;
    // 4 * 8 = 32
    uint32_t sp_top;
};

static inline struct cpuvar *arch_cpuvar_get(void) {
    uint32_t tp;
    __asm__ __volatile__("mv %0, tp" : "=r"(tp));
    return (struct cpuvar *) tp;
}

static inline void *arch_paddr2ptr(paddr_t paddr) {
    return (void *) paddr;
}
