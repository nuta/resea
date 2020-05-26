#ifndef __ARCH_H__
#define __ARCH_H__

#include <types.h>

#define TICK_HZ 1000
#define IRQ_MAX 32
#define STRAIGHT_MAP_ADDR 0 // Unused.
#define STRAIGHT_MAP_END  0 // Unused.

struct vm {
};

struct arch_task {
    void *stack_bottom;
    vaddr_t stack;
};

static inline void *from_paddr(paddr_t addr) {
    return (void *) addr;
}

static inline paddr_t into_paddr(void *addr) {
    return (vaddr_t) addr;
}

static inline bool is_kernel_addr_range(vaddr_t base, size_t len) {
    return false;
}

static inline bool is_kernel_paddr(paddr_t paddr) {
    return false;
}

static inline int mp_self(void) {
    return 0;
}

static inline bool mp_is_bsp(void) {
    return mp_self() == 0;
}


struct arch_cpuvar {
};

extern struct cpuvar cpuvar;
static inline struct cpuvar *get_cpuvar(void) {
    return &cpuvar;
}

#endif
