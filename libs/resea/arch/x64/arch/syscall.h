#ifndef __ARCH_SYSCALL_H__
#define __ARCH_SYSCALL_H__

#include <types.h>

static inline long syscall(int n, long a1, long a2, long a3, long a4, long a5) {
    long ret;
	register long r8 __asm__("r8") = a4;
	register long r9 __asm__("r9") = a5;
	register long r10 __asm__("r10") = a3;

    __asm__ __volatile__(
        "syscall"
        : "=a"(ret)
        : "D"(n), "S"(a1), "d"(a2), "r"(r10), "r"(r8), "r"(r9)
        : "rcx", "r11", "memory");

    return ret;
}

#endif
