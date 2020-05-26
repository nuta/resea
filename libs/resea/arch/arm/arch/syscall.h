#ifndef __ARCH_SYSCALL_H__
#define __ARCH_SYSCALL_H__

#include <types.h>

static inline long syscall(int n, long a1, long a2, long a3, long a4, long a5) {
    register long r0 __asm__("r0") = n;
    register long r1 __asm__("r1") = a1;
    register long r2 __asm__("r2") = a2;
    register long r3 __asm__("r3") = a3;
    register long r4 __asm__("r4") = a4;
    register long r5 __asm__("r5") = a5;
    register long r6 __asm__("r6") = r6;

    __asm__ __volatile__(
        "svc 0"
        : "=r"(r0), "=r"(r6)
        : "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5)
        : "ip", "memory");

    return r6;
}

#endif
