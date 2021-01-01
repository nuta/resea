#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

#include <types.h>

#define EXP_DEVICE_NOT_AVAILABLE 7
#define EXP_PAGE_FAULT           14

/// The interrupt stack frame.
struct iframe {
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
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
    uint64_t rdi;
    uint64_t error;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} __packed;

void x64_handle_interrupt(uint8_t vec, struct iframe *frame);
long x64_handle_syscall(long n, long a1, long a2, long a3, long a4, long a5);
struct task;
void release_task_irq(struct task *task);
void interrupt_init(void);

#endif
