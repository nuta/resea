#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <color.h>

// Declare here instead of including header files to allow inline functions
// in header files to use these macros.
void arch_panic(void);
void backtrace(void);

void printk(const char *fmt, ...);

#ifdef RELEASE_BUILD
#    define TRACE(fmt, ...)
#    define TRACE(fmt, ...)
#else
#    define TRACE(fmt, ...) \
        printk("[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#    define DEBUG(fmt, ...) \
        printk(COLOR_CYAN "[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#endif

#define INFO(fmt, ...) printk(COLOR_BLUE "[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#define WARN(fmt, ...) \
    printk(COLOR_YELLOW "[kernel] WARN: " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#define OOPS(fmt, ...)                                              \
    do {                                                            \
        printk(COLOR_YELLOW "[kernel] Oops: " fmt COLOR_RESET "\n", \
            ##__VA_ARGS__);                                         \
        backtrace();                                                \
    } while (0)

#define BUG(fmt, ...)                                                \
    do {                                                             \
        printk(COLOR_BOLD_RED "[kernel] BUG: " fmt COLOR_RESET "\n", \
            ##__VA_ARGS__);                                          \
        backtrace();                                                 \
        arch_panic();                                                \
    } while (0)

#define PANIC(fmt, ...)                                                \
    do {                                                               \
        printk(COLOR_BOLD_RED "[kernel] PANIC: " fmt COLOR_RESET "\n", \
            ##__VA_ARGS__);                                            \
        backtrace();                                                   \
        arch_panic();                                                  \
    } while (0)

#define UNIMPLEMENTED() \
    PANIC("Not yet implemented: %s:%d", __FILE__, __LINE__)

#define ASSERT(expr)                                             \
    do {                                                         \
        if (!(expr)) {                                           \
            printk(COLOR_BOLD_RED "[kernel] ASSERTION FAILURE: " \
                   #expr "\n" COLOR_RESET);                      \
            backtrace();                                         \
            arch_panic();                                        \
        }                                                        \
    } while (0)

#ifdef DEBUG_BUILD
#define DEBUG_ASSERT ASSERT
#else // DEBUG_BUILD
#define DEBUG_ASSERT(expr)
#endif

#endif
