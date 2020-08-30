#ifndef __TYPES_H__
#define __TYPES_H__

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;

typedef int task_t;
typedef long handle_t;

typedef long msec_t;
#define MSEC_MAX 0xffffffff

#if __LP64__
typedef unsigned long long offset_t;
#else
typedef unsigned long offset_t;
#endif

typedef char bool;
#define true 1
#define false 0
#define NULL ((void *) 0)

#define PAGE_SIZE 4096

typedef __builtin_va_list va_list;
#define offsetof(type, field)    __builtin_offsetof(type, field)
#define is_constant(expr)        __builtin_constant_p(expr)
#define va_start(ap, param)      __builtin_va_start(ap, param)
#define va_end(ap)               __builtin_va_end(ap)
#define va_arg(ap, type)         __builtin_va_arg(ap, type)
#define __unused                 __attribute__((unused))
#define __packed                 __attribute__((packed))
#define __noreturn               __attribute__((noreturn))
#define __weak                   __attribute__((weak))
#define __mustuse                __attribute__((warn_unused_result))
#define MEMORY_BARRIER()         __asm__ __volatile__("" ::: "memory");
#define __aligned(aligned_to)    __attribute__((aligned(aligned_to)))
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

// Error values. Don't forget to update `error_names` as well!
typedef int error_t;
#define IS_ERROR(err) ((err) < 0)
#define IS_OK(err)    ((err) >= 0)
#define OK                 (0)
#define ERR_NO_MEMORY      (-1)
#define ERR_NOT_PERMITTED  (-2)
#define ERR_WOULD_BLOCK    (-3)
#define ERR_ABORTED        (-4)
#define ERR_TOO_LARGE      (-5)
#define ERR_TOO_SMALL      (-6)
#define ERR_NOT_FOUND      (-7)
#define ERR_INVALID_ARG    (-8)
#define ERR_INVALID_TASK   (-9)
#define ERR_ALREADY_EXISTS (-10)
#define ERR_UNAVAILABLE    (-11)
#define ERR_NOT_ACCEPTABLE (-12)
#define ERR_EMPTY          (-13)
#define DONT_REPLY         (-14)
#define ERR_IN_USE         (-15)
#define ERR_TRY_AGAIN      (-16)
#define ERR_END            (-17)

// System call numbers.
#define SYS_EXEC    1
#define SYS_IPC     2
#define SYS_LISTEN  3
#define SYS_MAP     4
#define SYS_PRINT   6
#define SYS_KDEBUG  7

// Task flags.
#define TASK_ALL_CAPS (1 << 0)
#define TASK_ABI_EMU  (1 << 1)

// Map flags.
#define MAP_UPDATE (1 << 0)
#define MAP_DELETE (1 << 1)
#define MAP_W      (1 << 2)

// IPC source task IDs.
#define IPC_ANY  0  /* So-called "open receive". */
#define IPC_DENY -1 /* Blocked in the IPC send phase. Internally used by kernel. */

// IPC options.
#define IPC_SEND    (1 << 0)
#define IPC_RECV    (1 << 1)
#define IPC_CALL    (IPC_SEND | IPC_RECV)
#define IPC_NOBLOCK (1 << 2)
#define IPC_NOTIFY  (1 << 3)
#define IPC_KERNEL  (1 << 4) /* Internally used by kernel. */

// Flags in the message type (m->type).
#define MSG_STR  (1 << 30)
#define MSG_OOL (1 << 29)
#define MSG_ID(type) ((type) & 0xffff)

// Notifications.
typedef uint8_t notifications_t;
#define NOTIFY_TIMER    (1 << 0)
#define NOTIFY_IRQ      (1 << 1)
#define NOTIFY_ABORTED  (1 << 2)
#define NOTIFY_ASYNC    (1 << 3)

// Page Fault exception error codes.
#define EXP_PF_PRESENT (1 << 0)
#define EXP_PF_WRITE   (1 << 1)
#define EXP_PF_USER    (1 << 2)

enum exception_type {
    EXP_GRACE_EXIT,
    EXP_INVALID_MSG_FROM_PAGER,
    EXP_INVALID_MEMORY_ACCESS,
    EXP_INVALID_OP,
    EXP_ABORTED_KERNEL_IPC,
};

/// The kernel sends messages (e.g. EXCEPTION_MSG and PAGE_FAULT_MSG) as this
/// task ID.
#define KERNEL_TASK 0
/// The initial task ID.
#define INIT_TASK 1

#include <arch_types.h>

typedef uintmax_t uintptr_t;
typedef intmax_t ptrdiff_t;

#endif
