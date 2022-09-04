#pragma once

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;

#define INT8_MIN   -128
#define INT16_MIN  -32768
#define INT32_MIN  -2147483648
#define INT64_MIN  -9223372036854775808LL
#define INT8_MAX   127
#define INT16_MAX  32767
#define INT32_MAX  2147483647
#define INT64_MAX  9223372036854775807LL
#define UINT8_MAX  255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295U
#define UINT64_MAX 18446744073709551615ULL

#ifdef __LP64__
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;
#else
typedef int32_t intmax_t;
typedef uint32_t uintmax_t;
#endif

typedef char bool;
#define true 1
#define false 0
#define NULL ((void *) 0)

typedef int error_t;
typedef int16_t task_t;
typedef uintmax_t size_t;
typedef uintmax_t paddr_t;
typedef uintmax_t vaddr_t;
typedef uintmax_t uaddr_t;
typedef uintmax_t uintptr_t;

typedef __builtin_va_list va_list;
#define va_start(ap, param)   __builtin_va_start(ap, param)
#define va_end(ap)            __builtin_va_end(ap)
#define va_arg(ap, type)      __builtin_va_arg(ap, type)
#define offsetof(type, field) __builtin_offsetof(type, field)
#define is_constant(expr)     __builtin_constant_p(expr)

#define __unused   __attribute__((unused))
#define __packed   __attribute__((packed))
#define __noreturn __attribute__((noreturn))
#define __weak     __attribute__((weak))
#define __mustuse  __attribute__((warn_unused_result))

#define __user

#define ALIGN_DOWN(value, align) ((value) & ~((align) -1))
#define ALIGN_UP(value, align)   ALIGN_DOWN((value) + (align) -1, align)
#define IS_ALIGNED(value, align) (((value) & ((align) -1)) == 0)
#define STATIC_ASSERT(expr)      _Static_assert(expr, #expr);

#define MAX(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) __a = (a);                                               \
        __typeof__(b) __b = (b);                                               \
        (__a > __b) ? __a : __b;                                               \
    })
#define MIN(a, b)                                                              \
    ({                                                                         \
        __typeof__(a) __a = (a);                                               \
        __typeof__(b) __b = (b);                                               \
        (__a < __b) ? __a : __b;                                               \
    })

#define IS_ERROR(err) ((err) < 0)

#define OK                 0
#define ERR_EXISTS         -1
#define ERR_TOO_MANY_TASKS -2
#define ERR_WOULD_BLOCK    -3
#define ERR_ABORTED        -4
#define ERR_INVALID_ARG    -5

// FIXME:
static inline const char *err2str(int err) {
    return "";
}
