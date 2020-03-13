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

typedef int tid_t;
typedef unsigned msec_t;
#define MSEC_MAX 0xffffffff

typedef char bool;
#define true 1
#define false 0
#define NULL ((void *) 0)

#define offsetof(type, field)    __builtin_offsetof(type, field)
#define is_constant(expr)        __builtin_constant_p(expr)
#define va_list                  __builtin_va_list
#define va_start(ap, param)      __builtin_va_start(ap, param)
#define va_end(ap)               __builtin_va_end(ap)
#define va_arg(ap, type)         __builtin_va_arg(ap, type)
#define UNUSED                   __attribute__((unused))
#define PACKED                   __attribute__((packed))
#define NORETURN                 __attribute__((noreturn))
#define WEAK                     __attribute__((weak))
#define ALIGNED(align)           __attribute__((aligned(align)))
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

typedef int error_t;
#define IS_ERROR(err) ((err) < 0)
#define IS_OK(err)    ((err) >= 0)

// Error values. Don't forget to update `error_names` as well!
#define OK                 (0)
#define ERR_NO_MEMORY      (-1)
#define ERR_NOT_PERMITTED  (-2)
#define ERR_WOULD_BLOCK    (-3)
#define ERR_ABORTED        (-4)
#define ERR_TOO_LARGE      (-5)
#define ERR_TOO_SMALL      (-6)
#define ERR_NOT_FOUND      (-7)
#define ERR_INVALID_ARG    (-8)
#define ERR_ALREADY_EXISTS (-9)
#define ERR_UNAVAILABLE    (-10)
#define ERR_NOT_ACCEPTABLE (-11)
#define ERR_EMPTY          (-12)
#define ERR_END            (-13)

#define SYSCALL_IPC     1
#define SYSCALL_IPCCTL  2
#define SYSCALL_TASKCTL 3
#define SYSCALL_IRQCTL  4
#define SYSCALL_KLOGCTL 5

// IPC options.
#define IPC_ANY     0 /* So-called "open receive". */
#define IPC_SEND    (1 << 0)
#define IPC_RECV    (1 << 1)
#define IPC_CALL    (IPC_SEND | IPC_RECV)
#define IPC_NOBLOCK (1 << 2)
#define IPC_NOTIFY  (1 << 3)
#define IPC_KERNEL  (1 << 4) /* Internally used by kernel. */

// Message Type (m->type).
#define MSG_BULK(offset, len) (((offset) << 16) | ((len) << 24))
// Returns *offsets* in the message of bulk pointer/length fields.
#define MSG_BULK_PTR(msg_type) (((msg_type) >> 16) & 0xff)
#define MSG_BULK_LEN(msg_type) (((msg_type) >> 24) & 0xff)

typedef uint32_t caps_t;
#define CAP_NONE 0
#define CAP_ALL  0xffffffff
#define CAP_TASK (1 << 0)
#define CAP_IPC  (1 << 1)
#define CAP_IO   (1 << 2)
#define CAP_KLOG (1 << 3)

typedef uint32_t notifications_t;
#define NOTIFY_TIMER    (1 << 0)
#define NOTIFY_IRQ      (1 << 1)
#define NOTIFY_ABORTED  (1 << 2)
#define NOTIFY_NEW_DATA (1 << 3)

enum exception_type {
    EXP_GRACE_EXIT,
    EXP_NO_KERNEL_MEMORY,
    EXP_INVALID_PAGE_FAULT_REPLY,
    EXP_INVALID_MEMORY_ACCESS,
    EXP_INVALID_OP,
    EXP_ABORTED_KERNEL_IPC,
};

/// The kernel sends messages (e.g. EXCEPTION_MSG and PAGE_FAULT_MSG) as this
/// task ID.
#define KERNEL_TASK_TID 0
/// The initial task ID.
#define INIT_TASK_TID 1

#include <arch_types.h>

#endif
