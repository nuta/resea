#ifndef __ARCH_TYPES_H__
#define __ARCH_TYPES_H__

#include <config.h>

typedef uint64_t pageattrs_t;
typedef uint64_t size_t;
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;
typedef uint64_t uintptr_t;

#define PAGE_SIZE     4096
#define PAGE_PRESENT  (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER     (1 << 2)

typedef uint64_t pagefault_t;
#define PF_PRESENT (1 << 0)
#define PF_WRITE   (1 << 1)
#define PF_USER    (1 << 2)

#ifdef ABI_EMU
struct abi_emu_frame {
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
} PACKED;
#endif

#endif
