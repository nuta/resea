#ifndef __ARCH_SYSCALL_H__
#define __ARCH_SYSCALL_H__

#include <types.h>

static inline long syscall(int n, long a1, long a2, long a3, long a4, long a5) {
    register long x0 __asm__("x0") = n;
    register long x1 __asm__("x1") = a1;
    register long x2 __asm__("x2") = a2;
    register long x3 __asm__("x3") = a3;
    register long x4 __asm__("x4") = a4;
    register long x5 __asm__("x5") = a5;

    __asm__ __volatile__("svc 0"
                         : "=r"(x0)
                         : "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5)
                         : "memory", "cc");

    return x0;
}

#endif
