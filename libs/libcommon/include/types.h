#ifndef __TYPES_H__
#define __TYPES_H__

#define offsetof(type, field) __builtin_offsetof(type, field)
#define va_list __builtin_va_list
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define STATIC_ASSERT(expr) _Static_assert(expr, #expr)
#define UNUSED __attribute__((unused))
#define MAYBE_UNUSED __attribute__((unused))
#define PACKED __attribute__((packed))
#define NORETURN __attribute__((noreturn))
#define UNREACHABLE __builtin_unreachable()
#define LIKELY(cond) __builtin_expect(cond, 1)
#define UNLIKELY(cond) __builtin_expect(cond, 0)
#define atomic_compare_and_swap __sync_bool_compare_and_swap
#define POW2(order) (1 << (order))
#define ALIGN_DOWN(value, align) ((value) & ~((align) - 1))
#define ALIGN_UP(value, align) ALIGN_DOWN(value + align - 1, align)
#define MAX(a, b)                \
    ({                           \
        __typeof__(a) __a = (a); \
        __typeof__(b) __b = (b); \
        (__a > __b) ? __a : __b; \
    })
#define MIN(a, b)                \
    ({                           \
        __typeof__(a) __a = (a); \
        __typeof__(b) __b = (b); \
        (__a < __b) ? __a : __b; \
    })

#define NULL ((void *) 0)

#define PAGE_SIZE 4096

// error_t values
#define OK (0)
#define ERR_INVALID_CID (-1)
#define ERR_OUT_OF_RESOURCE (-2)
#define ERR_ALREADY_RECEVING (-3)
#define ERR_INVALID_HEADER (-4)
#define ERR_INVALID_PAYLOAD (-5)
#define ERR_INVALID_MESSAGE (-6)
#define ERR_NO_LONGER_LINKED (-7)
#define ERR_CHANNEL_CLOSED (-8)
#define ERR_OUT_OF_MEMORY (-9)
#define ERR_INVALID_SYSCALL (-10)
#define ERR_UNACCEPTABLE_PAGE_PAYLOAD (-11)
#define ERR_INVALID_NOTIFY_OP (-12)
#define ERR_UNIMPLEMENTED (-13)
#define ERR_UNEXPECTED_MESSAGE (-14)
#define ERR_NOT_FOUND (-64)
#define ERR_TOO_SHORT (-65)
#define ERR_INVALID_DATA (-66)
#define ERR_DONT_REPLY (-67)
#define ERR_NO_MEMORY (-68)

typedef int bool;
#define true 1
#define false 0

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;

#if __LP64__
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;
typedef uint64_t uintptr_t;
typedef uint64_t size_t;
typedef uint64_t uintmax_t;
typedef int64_t intmax_t;
#else
#    error "32-bit CPU is not yet supported"
#endif

typedef int16_t error_t;
typedef int32_t pid_t;
typedef int32_t tid_t;
typedef int32_t cid_t;

#endif
