#ifndef __X64_ASM_H__
#define __X64_ASM_H__

#include <types.h>

static inline uint64_t asm_read_rflags(void) {
    uint64_t rflags;
    __asm__ __volatile__("pushfq; popq %%rax" : "=a"(rflags));
    return rflags;
}

static inline void asm_write_rflags(uint64_t rflags) {
    __asm__ __volatile__("pushq %%rax; popfq" :: "a"(rflags));
}

static inline void asm_out8(uint16_t port, uint8_t value) {
    __asm__ __volatile__("outb %0, %1" :: "a"(value), "Nd"(port));
}

static inline uint8_t asm_in8(uint16_t port) {
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline void asm_stihlt(void) {
    __asm__ __volatile__("sti; hlt");
}

static inline void asm_cli(void) {
    __asm__ __volatile__("cli");
}

static inline void asm_hlt(void) {
    __asm__ __volatile__("hlt");
}

static inline void asm_lgdt(uint64_t gdtr) {
    __asm__ __volatile__("lgdt (%%rax)" :: "a"(gdtr));
}

static inline void asm_lidt(uint64_t idtr) {
    __asm__ __volatile__("lidt (%%rax)" :: "a"(idtr));
}

static inline void asm_ltr(uint16_t tr) {
    __asm__ __volatile__("ltr %0" :: "a"(tr));
}

static inline uint64_t asm_read_cr2(void) {
    uint64_t value;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(value));
    return value;
}

static inline void asm_write_cr3(uint64_t value) {
    __asm__ __volatile__("mov %0, %%cr3" :: "r"(value) : "memory");
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
    // Specify "memory" in clobber list to prevent memory
    // access reordering.
    __asm__ __volatile__("invlpg (%0)" :: "b"(vaddr) : "memory");
}

static inline void asm_pause(void) {
    __asm__ __volatile__("pause");
}
#endif
