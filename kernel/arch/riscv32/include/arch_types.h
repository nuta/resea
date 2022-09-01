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
    uint32_t sscratch;
    uint32_t sp;
};

// FIXME:
extern struct cpuvar cpuvar_fixme;

static inline struct cpuvar *arch_cpuvar_get(void) {
    return &cpuvar_fixme;
}

static inline void *arch_paddr2ptr(paddr_t paddr) {
    // FIXME:
    return (void *) paddr;
}
