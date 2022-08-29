#pragma once
#include <types.h>

#define PAGE_SIZE 4096

struct arch_task {};

struct arch_vm {
    paddr_t table;
};

static inline void *arch_paddr2ptr(paddr_t paddr) {
    // FIXME:
    return (void *) paddr;
}
