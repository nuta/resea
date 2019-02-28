#ifndef __TYPES_H__
#define __TYPES_H__

#define NULL ((void *) 0)
#define offsetof __builtin_offsetof
#define va_list __builtin_va_list
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap) __builtin_va_end(ap)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

#define STATIC_ASSERT _Static_assert
#define UNUSED __attribute__((unused))
#define PACKED __attribute__((packed))
#define NORETURN __attribute__((noreturn))
#define UNREACHABLE __builtin_unreachable()
#define atomic_compare_and_swap __sync_bool_compare_and_swap

#define ALIGN_DOWN(value, align) ((value) & ~((align) - 1))

// error_t values
#define OK                   (0)
#define ERR_INVALID_CID      (-1)
#define ERR_OUT_OF_RESOURCE  (-2)
#define ERR_ALREADY_RECEVING (-3)
#define ERR_INVALID_HEADER   (-4)
#define ERR_INVALID_MESSAGE  (-5)

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;
typedef int error_t;

#if __LP64__
typedef uint64_t paddr_t;
typedef uint64_t vaddr_t;
typedef uint64_t size_t;
typedef uint64_t uintmax_t;
typedef int64_t  intmax_t;
#else
#error "32-bit CPU is not yet supported"
#endif

#endif
