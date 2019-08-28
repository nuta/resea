#ifndef __RESEA_TYPES_H__
#    define __RESEA_TYPES_H__

#    define NULL ((void *) 0)
#    define STATIC_ASSERT(expr) _Static_assert(expr, #expr)
#    define UNUSED __attribute__((unused))
#    define PACKED __attribute__((packed))
#    define NORETURN __attribute__((noreturn))
#    define UNREACHABLE __builtin_unreachable()
#    define offsetof __builtin_offsetof
#    define va_list __builtin_va_list
#    define va_start(ap, param) __builtin_va_start(ap, param)
#    define va_end(ap) __builtin_va_end(ap)
#    define va_arg(ap, type) __builtin_va_arg(ap, type)

typedef char bool;
#    define false 0
#    define true 1

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;

#    if __LP64__
typedef uint64_t uintptr_t;
typedef uint64_t size_t;
typedef uint64_t uintmax_t;
typedef int64_t intmax_t;
#    else
typedef uint32_t size_t;
typedef uint32_t uintmax_t;
typedef int32_t intmax_t;
#    endif

typedef int32_t pid_t;
typedef int32_t tid_t;
typedef int32_t cid_t;

typedef int16_t error_t;
// error_t values
#    define OK (0)
#    define ERR_INVALID_CID (-1)
#    define ERR_OUT_OF_RESOURCE (-2)
#    define ERR_ALREADY_RECEVING (-3)
#    define ERR_INVALID_HEADER (-4)
#    define ERR_INVALID_PAYLOAD (-5)
#    define ERR_INVALID_MESSAGE (-6)
#    define ERR_NOT_FOUND (-64)
#    define ERR_TOO_SHORT (-65)
#    define ERR_INVALID_DATA (-66)
#    define ERR_DONT_REPLY (-67)
#    define ERR_NO_MEMORY (-68)

#endif
