#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

#include <printk.h>
#include <x64/x64.h>

#define KERNEL_BASE_ADDR        0xffff800000000000
#define KERNEL_BOOT_STACKS_ADDR 0xffff800000a00000
#define KERNEL_BOOT_STACKS_LEN  0x0000000000100000
#define CPU_VAR_ADDR            0xffff800000b00000
#define CPU_VAR_LEN             0x0000000000100000
#define SMALL_ARENA_ADDR        0xffff800001000000
#define SMALL_ARENA_LEN         0x0000000000400000
#define PAGE_ARENA_ADDR         0xffff800001400000
#define PAGE_ARENA_LEN          0x0000000000c00000

#define INIT_ARGS_OFFSET 256
#define INITFS_ADDR 0x0000000001000000
#define INITFS_END \
    0x0000000002000000 // TODO: make sure that memmgr.bin is not too big
#define STRAIGHT_MAP_ADDR       0x0000000003000000
#define STRAIGHT_MAP_END        0xffff800000000000

#define THREAD_INFO_ADDR        0x0000000000f1b000
#define ASAN_SHADOW_MEMORY 0xffff800010000000
#define OBJ_MAX_SIZE 1024
#define TICK_HZ 1000
#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER (1 << 2)
#define PF_PRESENT (1 << 0)
#define PF_WRITE (1 << 1)
#define PF_USER (1 << 2)

struct thread;
struct cpuvar {
    struct thread *idle_thread;
    struct gdtr gdtr;
    struct gdtr idtr;
    struct tss tss;
    uint64_t ioapic;
    uint64_t gdt[GDT_DESC_NUM];
    struct idt_entry idt[IDT_DESC_NUM];
    struct thread *current_fpu_owner;
};

typedef uint64_t flags_t;
typedef uint8_t spinlock_t;

struct arch_thread {
    // IMPORTANT NOTE: Don't forget to update offsets in switch.S and handler.S
    //  as well!
    uint64_t rsp;
    uint64_t kernel_stack;
    /// These values are intentionally redundant (they are copied from struct
    /// thread/process) in order to reduce cache pollution in IPC fastpath.
    uint64_t cr3;
    uint64_t info;
    uint64_t xsave_area;
    uint64_t rflags_ormask;
} PACKED;

struct page_table {
    // The physical address points to a PML4.
    paddr_t pml4;
};

static inline void *from_paddr(paddr_t addr) {
    return (void *) (addr + KERNEL_BASE_ADDR);
}

static inline paddr_t into_paddr(void *addr) {
    return (paddr_t)((uintmax_t) addr - KERNEL_BASE_ADDR);
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

static inline struct thread *get_current_thread(void) {
    struct thread *thread;
    __asm__ __volatile__("rdgsbase %0" : "=r"(thread));
    return thread;
}

static inline void *inlined_memset(void *dst, int ch, size_t len) {
    __asm__ __volatile__("cld; rep stosb" ::"D"(dst), "a"(ch), "c"(len));
    return dst;
}

static inline void *inlined_memcpy(void *dst, void *src, size_t len) {
    __asm__ __volatile__("cld; rep movsb" ::"D"(dst), "S"(src), "c"(len));
    return dst;
}

static inline bool spin_try_lock(spinlock_t *lock) {
    return atomic_compare_and_swap(lock, 0, 1);
}

static inline void spin_unlock(spinlock_t *lock) {
    __atomic_store_n(lock, 0, __ATOMIC_SEQ_CST);
}

static inline char *strcpy(char *dst, size_t dst_len, const char *src) {
    ASSERT(dst != NULL && "copy to NULL");
    ASSERT(src != NULL && "copy from NULL");

    size_t i = 0;
    while (i < dst_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
    return dst;
}

static inline void arch_idle(void) {
    // Wait for an interrupt.
    __asm__ __volatile__("sti;hlt");
}

static inline void spin_lock_init(spinlock_t *lock) {
    __atomic_store_n(lock, 0, __ATOMIC_SEQ_CST);
}

static inline void spin_lock(spinlock_t *lock) {
    while (!atomic_compare_and_swap(lock, 0, 1)) {
        asm_pause();
    }
}

static inline flags_t spin_lock_irqsave(spinlock_t *lock) {
    flags_t flags = asm_read_rflags();
    asm_cli();

    spin_lock(lock);
    return flags;
}

static inline void spin_unlock_irqrestore(spinlock_t *lock, flags_t flags) {
    spin_unlock(lock);
    asm_write_rflags(flags);
}

struct stack_frame {
    struct stack_frame *next;
    uint64_t return_addr;
} PACKED;

static inline struct stack_frame *get_stack_frame(void) {
    return (struct stack_frame *) __builtin_frame_address(0);
}

static inline bool is_valid_page_base_addr(vaddr_t page_base) {
    return page_base != 0 && page_base < KERNEL_BASE_ADDR;
}

#endif
