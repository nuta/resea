#pragma once
#include <types.h>

struct arch_task {};

struct arch_vm {
    paddr_t table;
};

static inline void *arch_paddr2ptr(paddr_t paddr) {
    // FIXME:
    return (void *) paddr;
}
