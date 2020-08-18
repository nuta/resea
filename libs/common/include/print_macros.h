#ifndef __PRINT_MACROS_H__
#define __PRINT_MACROS_H__

#include <types.h>
#include <vprintf.h>

void halt(void);

#ifdef KERNEL
#    define printf printk
void printk(const char *fmt, ...);
void panic_lock(void);
#else
void printf(const char *fmt, ...);
__noreturn void task_exit(void);
static inline void panic_lock(void) {}
#endif

const char *__program_name(void);

// ANSI escape sequences (SGR).
#define SGR_ERR      "\e[1;91m"  // Bold red.
#define SGR_WARN     "\e[0;33m"  // Yellow.
#define SGR_WARN_DBG "\e[1;33m"  // Bold yellow.
#define SGR_INFO     "\e[0;96m"  // Cyan.
#define SGR_DEBUG    "\e[1;32m"  // Bold green.
#define SGR_RESET    "\e[0m"

#define TRACE(fmt, ...)                                                        \
    do {                                                                       \
        printf("[%s] " fmt "\n", __program_name(), ##__VA_ARGS__);             \
    } while (0)

#define DBG(fmt, ...)                                                          \
    do {                                                                       \
        printf(SGR_DEBUG "[%s] " fmt SGR_RESET "\n", __program_name(),         \
               ##__VA_ARGS__);                                                 \
    } while (0)

#define INFO(fmt, ...)                                                         \
    do {                                                                       \
        printf(SGR_INFO "[%s] " fmt SGR_RESET "\n", __program_name(),          \
               ##__VA_ARGS__);                                                 \
    } while (0)

#define WARN(fmt, ...)                                                         \
    do {                                                                       \
        printf(SGR_WARN "[%s] WARN: " fmt SGR_RESET "\n", __program_name(),    \
               ##__VA_ARGS__);                                                 \
    } while (0)

#define WARN_DBG(fmt, ...)                                                     \
    do {                                                                       \
        printf(SGR_WARN_DBG "[%s] WARN: " fmt SGR_RESET "\n", __program_name(),\
               ##__VA_ARGS__);                                                 \
    } while (0)


#define HEXDUMP(ptr, len)                                                      \
    do {                                                                       \
        uint8_t *__ptr = (uint8_t *) (ptr);                                    \
        size_t __len = len;                                                    \
        printf("[%s] %04x: ", __program_name(), 0);                            \
        size_t i;                                                              \
        for (i = 0; i < __len; i++) {                                          \
            if (i > 0 && i % 16 == 0) {                                        \
                printf("\n");                                                  \
                printf("[%s] %04x: ", __program_name(), i);                    \
            }                                                                  \
                                                                               \
            printf("%02x ", __ptr[i]);                                         \
        }                                                                      \
                                                                               \
        printf("\n");                                                          \
    } while (0)

#define ASSERT(expr)                                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            printf(SGR_ERR "[%s] %s:%d: ASSERTION FAILURE: %s" SGR_RESET "\n", \
                   __program_name(),                                           \
                   __FILE__,                                                   \
                   __LINE__,                                                   \
                   #expr);                                                     \
            backtrace();                                                       \
            halt();                                                            \
            __builtin_unreachable();                                           \
        }                                                                      \
    } while (0)

#define DEBUG_ASSERT(expr) ASSERT(expr)

#define ASSERT_OK(expr)                                                        \
    do {                                                                       \
        __typeof__(expr) __expr = (expr);                                      \
        if (IS_ERROR(__expr)) {                                                \
            printf(SGR_ERR "[%s] %s:%d: unexpected error (%s)\n" SGR_RESET,    \
                   __program_name(),                                           \
                   __FILE__,                                                   \
                   __LINE__,                                                   \
                   err2str(__expr));                                           \
            backtrace();                                                       \
            halt();                                                            \
            __builtin_unreachable();                                           \
        }                                                                      \
    } while (0)

#define OOPS(fmt, ...)                                                         \
    do {                                                                       \
        printf(SGR_WARN "[%s] Oops: " fmt SGR_RESET "\n", __program_name(),    \
               ##__VA_ARGS__);                                                 \
        backtrace();                                                           \
    } while (0)

#define OOPS_OK(expr)                                                          \
    do {                                                                       \
        __typeof__(expr) __expr = (expr);                                      \
        if (IS_ERROR(__expr)) {                                                \
            printf(SGR_WARN "[%s] Oops: unexpected error (%s)\n" SGR_RESET,    \
                   __program_name(), err2str(__expr));                         \
            backtrace();                                                       \
        }                                                                      \
    } while (0)

#define PANIC(fmt, ...)                                                        \
    do {                                                                       \
        panic_lock();                                                          \
        printf(SGR_ERR "[%s] PANIC: " fmt SGR_RESET "\n", __program_name(),    \
               ##__VA_ARGS__);                                                 \
        backtrace();                                                           \
        halt();                                                                \
        __builtin_unreachable();                                               \
    } while (0)

#define NYI()                                                                  \
    do {                                                                       \
        panic_lock();                                                          \
        printf(SGR_ERR "[%s] %s(): not yet ymplemented: %s:%d\n",              \
               __program_name(), __func__, __FILE__, __LINE__);                \
        halt();                                                                \
        __builtin_unreachable();                                               \
    } while (0)

#define UNREACHABLE()                                                          \
    do {                                                                       \
        printf(SGR_ERR "Unreachable at %s:%d (%s)\n" SGR_RESET, __FILE__,      \
               __LINE__, __func__);                                            \
        backtrace();                                                           \
        halt();                                                                \
        __builtin_unreachable();                                               \
    } while (0)


#if !defined(CONFIG_BUILD_DEBUG)
#    undef DEBUG_ASSERT
#    define DEBUG_ASSERT(expr)
#    undef TRACE
#    define TRACE(fmt, ...)
#    undef WARN_DBG
#    define WARN_DBG(fmt, ...)
#endif

#endif
