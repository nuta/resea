#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

#include <support/printk.h>
#include <x64/serial.h>
#include <x64/x64.h>

#define KERNEL_BASE_ADDR        0xffff800000000000
#define KERNEL_BOOT_STACKS_ADDR 0xffff800000a00000
#define KERNEL_BOOT_STACKS_LEN  0x0000000000100000
#define CPU_VAR_ADDR            0xffff800000b00000
#define CPU_VAR_LEN             0x0000000000100000
#define OBJECT_ARENA_ADDR       0xffff800001000000
#define OBJECT_ARENA_LEN        0x0000000000400000
#define PAGE_ARENA_ADDR         0xffff800001400000
#define PAGE_ARENA_LEN          0x0000000000c00000
#define BOOTINFO_OFFSET        256
#define INITFS_ADDR             0x0000000001000000 // Initfs is gurannteed not
#define INITFS_END              0x0000000002000000 // to be larger than 16MiB.
#define STRAIGHT_MAP_ADDR       0x0000000003000000
#define STRAIGHT_MAP_END        0xffff800000000000
#define THREAD_INFO_ADDR        0x0000000000f1b000
#define ASAN_SHADOW_MEMORY      0xffff800010000000
#define OBJ_MAX_SIZE  256
#define TICK_HZ       1000
#define PAGE_SIZE     4096
#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER     (1 << 2)
#define PF_PRESENT    (1 << 0)
#define PF_WRITE      (1 << 1)
#define PF_USER       (1 << 2)
#define CPUVAR            (x64_get_cpuvar(arch_get_cpu_id()))
#define CPUVAR_OF(cpu_id) (x64_get_cpuvar(cpu_id))

unsigned x64_num_cpus(void);
#define NUM_CPUS  (x64_num_cpus())

struct thread;
struct cpuvar {
    struct thread *idle_thread;
    struct gdtr gdtr;
    struct gdtr idtr;
    struct tss tss;
    uint64_t gdt[GDT_DESC_NUM];
    struct idt_entry idt[IDT_DESC_NUM];
    struct thread *current_fpu_owner;
};

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

static inline uint8_t arch_get_cpu_id(void) {
    uint64_t apic_reg_id = 0xfee00020 + KERNEL_BASE_ADDR;
    return *((volatile uint32_t *) apic_reg_id) >> 24;
}

static inline struct cpuvar *x64_get_cpuvar(unsigned cpu_id) {
    struct cpuvar *cpuvars = (struct cpuvar *) CPU_VAR_ADDR;
    return &cpuvars[cpu_id];
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
    __asm__ __volatile__("cld; rep stosb"
        : "=D"(dst), "=a"(ch), "=c"(len)
        : "D"(dst), "a"(ch), "c"(len)
        : "memory");
    return dst;
}

static inline void *inlined_memcpy(void *dst, void *src, size_t len) {
    __asm__ __volatile__("cld; rep movsb"
        : "=D"(dst), "=S"(src), "=c"(len)
        : "D"(dst), "S"(src), "c"(len)
        : "memory");
    return dst;
}

static inline void arch_idle(void) {
    // Wait for an interrupt.
    __asm__ __volatile__("sti;hlt");
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

static inline void arch_putchar(char ch) {
    // Insert '\r' for serial console on QEMU.
    if (ch == '\n') {
        x64_serial_putchar('\r');
    }

    x64_serial_putchar(ch);
}

static inline error_t arch_get_screen_buffer(paddr_t *page, size_t *num_pages) {
    *page = 0xb8000;
    *num_pages = 1;
    return OK;
}

static inline uint64_t arch_read_ioport(uintmax_t addr, int size) {
    uint64_t data = 0;
    switch (size) {
    case 1: data = asm_in8(addr); break;
    case 2: data = asm_in16(addr); break;
    case 4: data = asm_in32(addr); break;
    default: PANIC("invalid ioport read size (%d)", size);
    }
    return data;
}

static inline void arch_write_ioport(uintmax_t addr, int size, uint64_t data) {
    switch (size) {
    case 1: asm_out8(addr, data); break;
    case 2: asm_out16(addr, data); break;
    case 4: asm_out32(addr, data); break;
    default: PANIC("invalid ioport write size (%d)", size);
    }
}

#endif
