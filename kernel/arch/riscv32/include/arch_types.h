#pragma once
#include <types.h>

#define PAGE_SIZE 4096

struct arch_task {
    uint32_t sp;
};

struct arch_vm {
    paddr_t table;
};

struct arch_cpuvar {
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
    uint32_t hartid;
};

// FIXME:
extern struct cpuvar cpuvar_fixme;

static inline struct cpuvar *arch_cpuvar_get(void) {
    return &cpuvar_fixme;
}

static inline void *arch_paddr2ptr(paddr_t paddr) {
    // 4 * = FIXME:
    return (void *) paddr;
}
