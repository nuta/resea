#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

#define mb()        __asm__ __volatile__("mfence")
#define CONSOLE_IRQ 4

typedef struct {
    uint64_t fsbase;
    uint64_t gsbase;
    uint64_t rip;
    uint64_t rflags;
    uint64_t rbp;
    uint64_t rax;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rsp;
} __packed trap_frame_t;

typedef struct {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    uint64_t rip;
    uint64_t cr3;
    uint8_t inst_len;
} hv_frame_t;

#endif
