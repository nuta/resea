#ifndef __ARCH_H__
#define __ARCH_H__

#include <types.h>

#define STACK_SIZE 4096
#define TICK_HZ 1000
#define IRQ_MAX 32
#define KERNEL_BASE_ADDR  0xffff000000000000
#define STRAIGHT_MAP_ADDR 0x03000000
#define STRAIGHT_MAP_END  0x3f000000

struct arch_task {
    vaddr_t syscall_stack;
    vaddr_t stack;
    void *syscall_stack_bottom;
    void *exception_stack_bottom;
    /// The level-0 page table.
    uint64_t *page_table;
    /// The user's page table paddr.
    paddr_t ttbr0;
};

static inline void *from_paddr(paddr_t addr) {
    return (void *) (addr + KERNEL_BASE_ADDR);
}

static inline paddr_t into_paddr(void *addr) {
    return ((vaddr_t) addr - KERNEL_BASE_ADDR);
}

static inline bool is_kernel_addr_range(vaddr_t base, size_t len) {
    // The first test checks whether an integer overflow occurs.
    return base + len < len || base >= KERNEL_BASE_ADDR
           || base + len >= KERNEL_BASE_ADDR;
}

// Note that these symbols point to *physical* addresses.
extern char __kernel_image[];
extern char __kernel_image_end[];

static inline bool is_kernel_paddr(paddr_t paddr) {
    return (paddr_t) __kernel_image <= paddr
            && paddr <= (paddr_t) __kernel_image_end;
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
