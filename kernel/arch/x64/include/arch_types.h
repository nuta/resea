#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

#include <x64/cpu.h>

#define KERNEL_BASE_ADDR        0xffff800000000000
#define CPU_VAR_ADDR            0xffff800000b00000
#define OBJ_POOL_ADDR           0xffff800001000000
#define KERNEL_STACK_POOL_ADDR  0xffff800001400000
#define PAGE_POOL_ADDR          0xffff800001800000
#define OBJ_POOL_LEN            0x400000
#define KERNEL_STACK_POOL_LEN   0x400000
#define PAGE_POOL_LEN           0x800000
#define INITFS_ADDR             0x0000000000100000
#define INITFS_END              0x0000000000200000
#define STRAIGHT_MAP_ADDR       0x0000000002000000
#define STRAIGHT_MAP_END        0xffffffffffffffff
#define OBJ_MAX_SIZE    1024
#define TICK_HZ         1000
#define PAGE_SIZE       4096
#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITABLE   (1 << 1)
#define PAGE_USER       (1 << 2)
#define PF_PRESENT      (1 << 0)
#define PF_WRITE        (1 << 1)
#define PF_USER         (1 << 2)

typedef uint64_t flags_t;
typedef uint8_t spinlock_t;

// Thread-local information accessible from the user.
struct arch_thread_info {
    uint64_t user_buffer;
} PACKED;

struct arch_thread {
    // IMPORTANT NOTE: Don't forget to update offsets in switch.S and handler.S
    //  as well!
    uint64_t rip;     // 0
    uint64_t rsp;     // 8
    uint64_t rsp0;    // 16: rsp0 == kernel_stack + (size of stack)
    uint64_t cr3;     // 24
    uint64_t info;    // 32
    uint64_t buffer;  // 40
    uint64_t rflags;  // 48
    uint64_t rflags_ormask; // 56
};

struct arch_page_table {
    // The physical address points to a PML4.
    paddr_t pml4;
};

static inline void *from_paddr(paddr_t addr) {
    return (void *) (addr + KERNEL_BASE_ADDR);
}

static inline paddr_t into_paddr(void *addr) {
    return (paddr_t) ((uintmax_t) addr - KERNEL_BASE_ADDR);
}

static inline uint8_t x64_read_cpu_id(void) {
    uint64_t apic_reg_id = 0xfee00020 + KERNEL_BASE_ADDR;
    return *((volatile uint32_t *) apic_reg_id) >> 24;
}

static inline struct cpuvar *x64_get_cpuvar(void) {
    struct cpuvar *cpuvars = (struct cpuvar *) CPU_VAR_ADDR;
    return &cpuvars[x64_read_cpu_id()];
}

static inline vaddr_t arch_get_stack_pointer(void) {
    uint64_t rsp;
    __asm__ __volatile__("movq %%rsp, %%rax" : "=a"(rsp));
    return rsp;
}

#endif
