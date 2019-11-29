#ifndef __TYPES_H__
#define __TYPES_H__

#define offsetof(type, field) __builtin_offsetof(type, field)
#define is_constant(expr) __builtin_constant_p(expr)
#define va_list __builtin_va_list
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define STATIC_ASSERT(expr) _Static_assert(expr, #expr)
#define UNUSED __attribute__((unused))
#define MAYBE_UNUSED __attribute__((unused))
#define PACKED __attribute__((packed))
#define WEAK __attribute__((weak))
#define NORETURN __attribute__((noreturn))
#define UNREACHABLE __builtin_unreachable()
#define LIKELY(cond) __builtin_expect(cond, 1)
#define UNLIKELY(cond) __builtin_expect(cond, 0)
#define atomic_swap(ptr, new_value) __sync_swap(ptr, new_value)
#define atomic_compare_and_swap(ptr, new, old) __sync_bool_compare_and_swap(ptr, new, old)
#define ALIGN_DOWN(value, align) ((value) & ~((align) - 1))
#define ALIGN_UP(value, align) ALIGN_DOWN((value) + (align) - 1, align)
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
#define INVALID_POINTER ((void *) 0xdeaddead)

// FIXME: This is arch-specific, isn't it?
#define PAGE_SIZE 4096

typedef int error_t;
enum error_codes {
    OK = 0x00,

    // Critical errors.
    ERR_OUT_OF_MEMORY   = 0x01,
    ERR_OUT_OF_RESOURCE = 0x02,
    ERR_UNIMPLEMENTED   = 0x03,

    // Errors from system calls.
    ERR_INVALID_CID          = 0x10,
    ERR_INVALID_MSG          = 0x11,
    ERR_INVALID_PAYLOAD      = 0x12,
    ERR_INVALID_SYSCALL      = 0x13,
    ERR_INVALID_PAGE_PAYLOAD = 0x14,
    ERR_CHANNEL_CLOSED       = 0x15,
    ERR_ALREADY_RECEVING     = 0x16,

    // General errors.
    ERR_UNKNOWN_MESSAGE = 0x30,
    ERR_BAD_REQUEST     = 0x31,
    ERR_NOT_FOUND       = 0x32,
    ERR_NOT_ACCEPTABLE  = 0x33,
    ERR_TOO_SHORT       = 0x34,
    ERR_TOO_LARGE       = 0x35,
    ERR_ALREDY_EXISTS   = 0x36,
    ERR_IM_A_TEAPOT     = 0x37,

    // Try again!
    ERR_WOULD_BLOCK = 0x50,
    ERR_NEEDS_RETRY = 0x51,

    // Used by servers to indicate that the server does not need to send a reply
    // message.
    NO_REPLY = 0x70,
};

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

typedef int32_t pid_t;
typedef int32_t tid_t;
typedef int32_t cid_t;

#endif
