#ifndef __X64_H__
#define __X64_H__

#include <config.h>
#include <types.h>

//
//  Global Descriptor Table (GDT)
//

// A TSS desc is twice as large as others.
#define GDT_DESC_NUM 8

#define GDT_NULL 0
#define GDT_KERNEL_CODE 1
#define GDT_KERNEL_DATA 2
#define GDT_USER_CODE32 3
#define GDT_USER_DATA 4
#define GDT_USER_CODE64 5
#define GDT_TSS 6

#define KERNEL_CS (GDT_KERNEL_CODE * 8)
#define TSS_SEG (GDT_TSS * 8)
#define USER_CS32 (GDT_USER_CODE32 * 8)
#define USER_CS64 (GDT_USER_CODE64 * 8)
#define USER_DS (GDT_USER_DATA * 8)
#define USER_RPL 3

struct gdtr {
    uint16_t len;
    uint64_t laddr;
} PACKED;

//
//  Interrupt Descriptor Table (IDT)
//
#define IDT_DESC_NUM 256
#define IDT_INT_HANDLER 0x8e
#define IST_RSP0 0

struct idt_entry {
    uint16_t offset1;
    uint16_t seg;
    uint8_t ist;
    uint8_t info;
    uint16_t offset2;
    uint32_t offset3;
    uint32_t reserved;
} PACKED;

struct idtr {
    uint16_t len;
    uint64_t laddr;
} PACKED;

//
//  Control Registers
//
#define CR0_TS       (1ul << 3)
#define CR4_FSGSBASE (1ul << 16)
#define CR4_OSXSAVE  (1ul << 18)

//
//  Model Specific Registers (MSR)
//
#define MSR_APIC_BASE 0x0000001b
#define MSR_PMC0 0x000000c1
#define MSR_PERFEVTSEL(n) (0x00000186 + (n))
#define MSR_PERF_GLOBAL_CTRL 0x0000038f
#define MSR_GS_BASE 0xc0000101
#define MSR_KERNEL_GS_BASE 0xc0000102
#define MSR_EFER 0xc0000080
#define MSR_STAR 0xc0000081
#define MSR_LSTAR 0xc0000082
#define MSR_CSTAR 0xc0000083
#define MSR_FMASK 0xc0000084
#define EFER_SCE 0x01

// Clear IF bit to disable interrupts when we enter the syscall handler
// or an interrupt occurs before doing SWAPGS.
#define SYSCALL_RFLAGS_MASK 0x200

//
//  Task State Segment (TSS)
//
struct tss {
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist[7];
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap;
} PACKED;

//
//  Page Table
//
#define PAGE_ENTRY_NUM 512
#define KERNEL_PML4_PADDR 0x00700000

//
//  Inline assembly.
//
static inline void asm_out8(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" ::"a"(value), "Nd"(port));
}

static inline void asm_out16(uint16_t port, uint16_t value) {
    __asm__ __volatile__("outw %0, %1" ::"a"(value), "Nd"(port));
}

static inline void asm_out32(uint16_t port, uint32_t value) {
    __asm__ __volatile__("outl %0, %1" ::"a"(value), "Nd"(port));
}

static inline uint8_t asm_in8(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint16_t asm_in16(uint16_t port) {
    uint16_t value;
    __asm__ __volatile__("inw %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline uint32_t asm_in32(uint16_t port) {
    uint32_t value;
    __asm__ __volatile__("inl %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void asm_cli(void) {
    __asm__ __volatile__("cli");
}

static inline void asm_lgdt(uint64_t gdtr) {
    __asm__ __volatile__("lgdt (%%rax)" ::"a"(gdtr));
}

static inline void asm_lidt(uint64_t idtr) {
    __asm__ __volatile__("lidt (%%rax)" ::"a"(idtr));
}

static inline void asm_ltr(uint16_t tr) {
    __asm__ __volatile__("ltr %0" ::"a"(tr));
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
    __asm__ __volatile__("mov %0, %%cr0" ::"r"(value));
}

static inline void asm_write_cr3(uint64_t value) {
    __asm__ __volatile__("mov %0, %%cr3" ::"r"(value));
}

static inline void asm_write_cr4(uint64_t value) {
    __asm__ __volatile__("mov %0, %%cr4" ::"r"(value));
}

static inline void asm_wrmsr(uint32_t reg, uint64_t value) {
    uint32_t low = value & 0xffffffff;
    uint32_t hi = value >> 32;
    __asm__ __volatile__("wrmsr" ::"c"(reg), "a"(low), "d"(hi));
}

static inline uint64_t asm_rdmsr(uint32_t reg) {
    uint32_t low, high;
    __asm__ __volatile__("rdmsr" : "=a"(low), "=d"(high) : "c"(reg));
    return ((uint64_t) high << 32) | low;
}

static inline void asm_invlpg(uint64_t vaddr) {
    __asm__ __volatile__("invlpg (%0)" ::"b"(vaddr) : "memory");
}

static inline void asm_pause(void) {
    __asm__ __volatile__("pause");
}

static inline void asm_wrgsbase(uint64_t gsbase) {
#ifdef CONFIG_X86_FSGSBASE
    __asm__ __volatile__("wrgsbase %0" ::"r"(gsbase));
#else
    asm_wrmsr(MSR_GS_BASE, gsbase);
#endif
}

static inline void asm_swapgs(void) {
    __asm__ __volatile__("swapgs");
}

static inline void asm_xsave(vaddr_t xsave_area) {
    __asm__ __volatile__("xsave (%0)" :: "r"(xsave_area) : "memory");
}

static inline void asm_xrstor(vaddr_t xsave_area) {
    __asm__ __volatile__("xrstor (%0)" :: "r"(xsave_area) : "memory");
}

static inline void asm_cpuid(uint32_t leaf, uint32_t *eax, uint32_t *ebx,
                             uint32_t *ecx, uint32_t *edx) {
    __asm__ __volatile__("cpuid"
        : "=a"(*eax), "=b" (*ebx), "=c"(*ecx), "=d"(*edx)
        : "0"(leaf));
}

static inline void asm_cpuid_count(uint32_t leaf, uint32_t subleaf,
                                   uint32_t *eax, uint32_t *ebx, uint32_t *ecx,
                                   uint32_t *edx) {
    __asm__ __volatile__("cpuid"
        : "=a"(*eax), "=b" (*ebx), "=c"(*ecx), "=d"(*edx)
        : "0"(leaf), "2"(subleaf));
}

#endif
