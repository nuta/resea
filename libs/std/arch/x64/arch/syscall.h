#ifndef __ARCH_SYSCALL_H__
#define __ARCH_SYSCALL_H__

#include <types.h>

static inline uint64_t syscall(uint64_t syscall, uint64_t arg1, uint64_t arg2,
                               uint64_t arg3, uint64_t arg4, uint64_t arg5) {
    uint64_t ret;
    __asm__ __volatile__(
        "movq %[arg3], %%r10   \n"
        "movq %[arg4], %%r8    \n"
        "movq %[arg5], %%r9    \n"
        "syscall               \n"
        : "=a"(ret), "+D"(syscall), "+S"(arg1), "+d"(arg2), [arg3] "+r"(arg3),
          [arg4] "+r"(arg4), [arg5] "+r"(arg5)::"memory", "%r8", "%r9", "%r10",
          "%r11");

    return ret;
}

#endif
