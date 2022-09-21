#pragma once

// Build config.
// TODO: Move to a separate file.
#define NUM_TASKS_MAX 32
#define NUM_CPUS_MAX  32
#define TASK_NAME_LEN 16

typedef char int8_t;
typedef short int16_t;
typedef int int32_t;
typedef long long int64_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned uint32_t;
typedef unsigned long long uint64_t;

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
typedef int task_t;
// FIXME: typedef _UnsignedBitInt(8 + NUM_TASKS_MAX) notifications_t;
typedef uint32_t notifications_t;
typedef uintmax_t size_t;
typedef uintmax_t paddr_t;
typedef uintmax_t vaddr_t;
typedef uintmax_t uaddr_t;
typedef uintmax_t uintptr_t;
typedef uintmax_t offset_t;

#define __unused              __attribute__((unused))
#define __packed              __attribute__((packed))
#define __noreturn            __attribute__((noreturn))
#define __weak                __attribute__((weak))
#define __mustuse             __attribute__((warn_unused_result))
#define __aligned(aligned_to) __attribute__((aligned(aligned_to)))

/// An attribute for a pointer given by the user. Don't dereference it directly:
/// access it through safe functions such as memcpy_from_user and
/// memcpy_to_user!
#define __user __attribute__((noderef, address_space(1)))

typedef __builtin_va_list va_list;
#define va_start(ap, param) __builtin_va_start(ap, param)
#define va_end(ap)          __builtin_va_end(ap)
#define va_arg(ap, type)    __builtin_va_arg(ap, type)

#define offsetof(type, field)    __builtin_offsetof(type, field)
#define ALIGN_DOWN(value, align) __builtin_align_down(value, align)
#define ALIGN_UP(value, align)   __builtin_align_up(value, align)
#define IS_ALIGNED(value, align) __builtin_is_aligned(value, align)
#define STATIC_ASSERT(expr)      _Static_assert(expr, #expr);

#define IS_OK(err)          (!IS_ERROR(err))
#define IS_ERROR(err)       (((long) (err)) < 0)
#define OK                  0
#define ERR_NO_MEMORY       -1
#define ERR_NO_RESOURCES    -2
#define ERR_NOT_FOUND       -3
#define ERR_ALREADY_EXISTS  -4
#define ERR_ALREADY_USED    -5
#define ERR_STILL_USED      -6
#define ERR_NOT_ALLOWED     -7
#define ERR_NOT_SUPPORTED   -8
#define ERR_INVALID_ARG     -9
#define ERR_INVALID_TASK    -10
#define ERR_INVALID_SYSCALL -11
#define ERR_TOO_MANY_TASKS  -12
#define ERR_TOO_LARGE       -13
#define ERR_TOO_SMALL       -14
#define ERR_WOULD_BLOCK     -15
#define ERR_TRY_AGAIN       -16
#define ERR_ABORTED         -17
#define ERR_END             -18

// The size of a memory page in bytes.
#define PAGE_SIZE 4096

// Remarkable task IDs.
#define KERNEL_TASK_ID -1
#define INIT_TASK_ID   1

// System call numbers.
#define SYS_IPC           1
#define SYS_NOTIFY        2
#define SYS_CONSOLE_WRITE 6
#define SYS_CONSOLE_READ  7
#define SYS_TASK_CREATE   8
#define SYS_TASK_DESTROY  9
#define SYS_TASK_EXIT     10
#define SYS_TASK_SELF     11
#define SYS_PM_ALLOC      12
#define SYS_VM_MAP        13
#define SYS_VM_UNMAP      14

// Page attributes.
#define PAGE_READABLE   (1 << 1)
#define PAGE_WRITABLE   (1 << 2)
#define PAGE_EXECUTABLE (1 << 3)
#define PAGE_USER       (1 << 4)

// Page fault reasons.
#define PAGE_FAULT_READ    (1 << 0)
#define PAGE_FAULT_WRITE   (1 << 1)
#define PAGE_FAULT_EXEC    (1 << 2)
#define PAGE_FAULT_USER    (1 << 3)
#define PAGE_FAULT_PRESENT (1 << 4)

// Exception codes.
#define EXCEPTION_GRACE_EXIT               1
#define EXCEPTION_INVALID_UADDR_ACCESS     2
#define EXCEPTION_INVALID_PAGE_FAULT_REPLY 3
