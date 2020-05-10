#ifndef __ARCH_H__
#define __ARCH_H__

#include <types.h>

#define IRQ_MAX   256
#define TIMER_IRQ 0

#define KERNEL_BASE_ADDR  0xffff800000000000
#define INITFS_ADDR       0x0000000001000000
#define INITFS_END        0x0000000002000000
#define STRAIGHT_MAP_ADDR 0x0000000003000000
#define STRAIGHT_MAP_END  0xffff800000000000

struct vm {
    paddr_t pml4;
};

struct arch_task {
    uint64_t rsp;
    uint64_t interrupt_stack;
    uint64_t syscall_stack;
    void *interrupt_stack_bottom;
    void *syscall_stack_bottom;
    void *xsave;
    uint64_t gsbase;
    uint64_t fsbase;
} PACKED;

static inline void *from_paddr(paddr_t addr) {
    return (void *) (addr + KERNEL_BASE_ADDR);
}

static inline paddr_t into_paddr(void *addr) {
    return ((vaddr_t) addr - KERNEL_BASE_ADDR);
}

/// Retuns whether the memory address range [base, base_len) overlaps with the
/// kernel address space.
static inline bool is_kernel_addr_range(vaddr_t base, size_t len) {
    // The first test checks whether an integer overflow occurs.
    return base + len < len || base >= KERNEL_BASE_ADDR
           || base + len >= KERNEL_BASE_ADDR;
}

// Note that these symbols point to *physical* addresses.
extern char __kernel_image[];
extern char __kernel_image_end[];
extern char __kernel_data[];
extern char __kernel_data_end[];

/// Retuns whether the memory address range [base, base_len) overlaps with the
/// kernel memory pages.
static inline bool is_kernel_paddr(paddr_t paddr) {
    return
        ((paddr_t) __kernel_image <= paddr
            && paddr <= (paddr_t) __kernel_image_end)
        || ((paddr_t) __kernel_data <= paddr
            && paddr <= (paddr_t) __kernel_data_end);
}

static inline int mp_self(void) {
    return *((volatile uint32_t *) from_paddr(0xfee00020)) >> 24;
}

static inline bool mp_is_bsp(void) {
    return mp_self() == 0;
}

//
//  Global Descriptor Table (GDT)
//
#define TSS_SEG   offsetof(struct gdt, tss_low)
#define KERNEL_CS offsetof(struct gdt, kernel_cs)
#define USER_CS32 offsetof(struct gdt, user_cs32)
#define USER_CS64 offsetof(struct gdt, user_cs64)
#define USER_DS   offsetof(struct gdt, user_ds)
#define USER_RPL  3

struct gdt {
    uint64_t null;
    uint64_t kernel_cs;
    uint64_t kernel_ds;
    uint64_t user_cs32;
    uint64_t user_ds;
    uint64_t user_cs64;
    uint64_t tss_low;
    uint64_t tss_high;
} PACKED;

struct gdtr {
    uint16_t len;
    uint64_t laddr;
} PACKED;

//
//  Interrupt Descriptor Table (IDT)
//
#define IDT_DESC_NUM    256
#define IDT_INT_HANDLER 0x8e
#define IST_RSP0        0

struct idt_desc {
    uint16_t offset1;
    uint16_t seg;
    uint8_t ist;
    uint8_t info;
    uint16_t offset2;
    uint32_t offset3;
    uint32_t reserved;
} PACKED;

struct idt {
    struct idt_desc descs[IDT_DESC_NUM];
} PACKED;

struct idtr {
    uint16_t len;
    uint64_t laddr;
} PACKED;

//
//  Control Registers
//
#define CR0_MP          (1ul << 1)
#define CR0_EM          (1ul << 2)
#define CR0_TS          (1ul << 3)
#define CR4_FSGSBASE    (1ul << 16)
#define CR4_OSXSAVE     (1ul << 18)
#define CR4_OSFXSR      (1ul << 9)
#define CR4_OSXMMEXCPT  (1ul << 10)

//
//  Extended Control Register 0 (XCR0)
//
#define XCR0_SSE (1ul << 1)
#define XCR0_AVX (1ul << 2)

//
//  Model Specific Registers (MSR)
//
#define MSR_APIC_BASE        0x0000001b
#define MSR_PERFEVTSEL(n)    (0x00000186 + (n))
#define MSR_PERF_GLOBAL_CTRL 0x0000038f
#define MSR_KERNEL_GS_BASE   0xc0000102
#define MSR_EFER             0xc0000080
#define MSR_STAR             0xc0000081
#define MSR_LSTAR            0xc0000082
#define MSR_CSTAR            0xc0000083
#define MSR_FMASK            0xc0000084
#define EFER_SCE             0x01

//
//  Task State Segment (TSS)
//
#define TSS_IOMAP_SIZE 8191
struct tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_offset;
    uint8_t iomap[TSS_IOMAP_SIZE];
    // According to Intel SDM, all bits of the last byte must be set to 1.
    uint8_t iomap_last_byte;
} PACKED;

//
//  Page Table
//
extern char __kernel_pml4[];  // paddr_t
#define PAGE_ENTRY_NUM 512

//
//  PIC
//
#define PIT_HZ            1193182
#define PIT_CH2           0x42
#define PIT_CMD           0x43
#define KBC_PORT_B        0x61
#define KBC_B_OUT2_STATUS 0x20

//
//  APIC
//
#define APIC_REG_ID                     0xfee00020
#define APIC_REG_VERSION                0xfee00030
#define APIC_REG_TPR                    0xfee00080
#define APIC_REG_EOI                    0xfee000b0
#define APIC_REG_LOGICAL_DEST           0xfee000d0
#define APIC_REG_DEST_FORMAT            0xfee000e0
#define APIC_REG_SPURIOUS_INT           0xfee000f0
#define APIC_REG_ICR_LOW                0xfee00300
#define APIC_REG_ICR_HIGH               0xfee00310
#define APIC_REG_LVT_TIMER              0xfee00320
#define APIC_REG_LINT0                  0xfee00350
#define APIC_REG_LINT1                  0xfee00360
#define APIC_REG_LVT_ERROR              0xfee00370
#define APIC_REG_TIMER_INITCNT          0xfee00380
#define APIC_REG_TIMER_CURRENT          0xfee00390
#define APIC_REG_TIMER_DIV              0xfee003e0
#define IOAPIC_IOREGSEL_OFFSET          0x00
#define IOAPIC_IOWIN_OFFSET             0x10
#define VECTOR_IPI_RESCHEDULE           32
#define VECTOR_IPI_HALT                 33
#define VECTOR_IRQ_BASE                 48
#define IOAPIC_ADDR                     0xfec00000
#define IOAPIC_REG_IOAPICVER            0x01
#define IOAPIC_REG_NTH_IOREDTBL_LOW(n)  (0x10 + ((n) *2))
#define IOAPIC_REG_NTH_IOREDTBL_HIGH(n) (0x10 + ((n) *2) + 1)

static inline uint32_t read_apic(paddr_t addr) {
    return *((volatile uint32_t *) from_paddr(addr));
}

static inline void write_apic(paddr_t addr, uint32_t data) {
    *((volatile uint32_t *) from_paddr(addr)) = data;
}

//
//  APIC Timer.
//
#define APIC_TIMER_DIV 0x03
#define TICK_HZ        1000

//
//  MP
//

// The maximum number of CPUs. Don't forget to expand the boot stack and
// CPU-local variables space defined in kernel.ld as well!
#define CPU_NUM_MAX     16
#define CPUVAR_SIZE_MAX 0x4000  // Don't forget to update kernel.ld as well!

extern char __cpuvar_base[];            // paddr_t
extern char __cpuvar_end[];             // paddr_t
extern char __mp_boot_trampoine[];      // paddr_t
extern char __mp_boot_trampoine_end[];  // paddr_t
extern char __mp_boot_gdtr[];           // paddr_t

/// CPU-local variables. Accessible through GS segment in kernel mode.
struct arch_cpuvar {
    uint64_t rsp0;
    struct gdt gdt;
    struct idt idt;
    struct tss tss;
};

struct cpuvar;
static inline struct cpuvar *get_cpuvar(void) {
    uint64_t gsbase;
    __asm__ __volatile__("rdgsbase %0" : "=r"(gsbase));
    return (struct cpuvar *) gsbase;
}

//
//  SYSCALL/SYSRET
//

// SYSRET constraints.
STATIC_ASSERT(USER_CS32 + 8 == USER_DS);
STATIC_ASSERT(USER_CS32 + 16 == USER_CS64);

// Clear IF bit to disable interrupts when we enter the syscall handler
// or an interrupt occurs before doing SWAPGS.
#define SYSCALL_RFLAGS_MASK 0x200

//
//  Inline assembly.
//
static inline void asm_out8(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" ::"a"(value), "Nd"(port));
}

static inline uint8_t asm_in8(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void asm_cli(void) {
    __asm__ __volatile__("cli");
}

static inline void asm_stihlt(void) {
    __asm__ __volatile__("sti; hlt");
}

// Disable clang-format temporarily because it does not handles "::" as desire.
// clang-format off
static inline void asm_lgdt(uint64_t gdtr) {
    __asm__ __volatile__("lgdt (%0)" :: "r"(gdtr));
}

static inline void asm_lidt(uint64_t idtr) {
    __asm__ __volatile__("lidt (%0)" :: "r"(idtr));
}

static inline void asm_ltr(uint16_t tr) {
    __asm__ __volatile__("ltr %0" :: "r"(tr));
}

static inline uint64_t asm_read_cr0(void) {
    uint64_t value;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(value));
    return value;
}

static inline uint64_t asm_read_cr2(void) {
    uint64_t value;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(value));
    return value;
}

static inline uint64_t asm_read_cr4(void) {
    uint64_t value;
    __asm__ __volatile__("mov %%cr4, %0" : "=r"(value));
    return value;
}

static inline void asm_write_cr0(uint64_t value) {
    __asm__ __volatile__("mov %0, %%cr0" :: "r"(value));
}

static inline void asm_write_cr3(uint64_t value) {
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(value));
}

static inline void asm_write_cr4(uint64_t value) {
    __asm__ __volatile__("mov %0, %%cr4" :: "r"(value));
}

static inline void asm_wrmsr(uint32_t reg, uint64_t value) {
    uint32_t low = value & 0xffffffff;
    uint32_t hi = value >> 32;
    __asm__ __volatile__("wrmsr" :: "c"(reg), "a"(low), "d"(hi));
}

static inline uint64_t asm_rdmsr(uint32_t reg) {
    uint32_t low, high;
    __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(reg));
    return ((uint64_t) high << 32) | low;
}

static inline void asm_invlpg(uint64_t vaddr) {
    __asm__ __volatile__("invlpg (%0)" :: "b"(vaddr) : "memory");
}

static inline void asm_swapgs(void) {
    __asm__ __volatile__("swapgs");
}

static inline uint64_t asm_rdgsbase(void) {
    uint64_t value;
    __asm__ __volatile__("rdgsbase %0" : "=r"(value));
    return value;
}

static inline uint64_t asm_rdfsbase(void) {
    uint64_t value;
    __asm__ __volatile__("rdfsbase %0" : "=r"(value));
    return value;
}

static inline void asm_wrgsbase(uint64_t gsbase) {
    __asm__ __volatile__("wrgsbase %0" :: "r"(gsbase));
}

static inline void asm_wrfsbase(uint64_t fsbase) {
    __asm__ __volatile__("wrfsbase %0" :: "r"(fsbase));
}

static inline void asm_xsave(void *xsave) {
    __asm__ __volatile__("xsave (%0)" :: "r"(xsave) : "memory");
}

static inline void asm_xrstor(void *xsave) {
    __asm__ __volatile__("xrstor (%0)" :: "r"(xsave) : "memory");
}

static inline uint64_t asm_xgetbv(uint32_t xcr) {
    uint32_t low, high;
    __asm__ __volatile__("xgetbv" : "=a"(low), "=d"(high) : "c"(xcr));
    return ((uint64_t) high << 32) | low;
}

static inline void asm_xsetbv(uint32_t xcr, uint64_t value) {
    __asm__ __volatile__("xsetbv" :: "a"(value), "c"(xcr));
}

// clang-format on

#endif
