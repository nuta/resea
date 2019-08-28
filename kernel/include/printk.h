#ifndef __PRINTK_H__
#define __PRINTK_H__

// Declare here instead of including header files to allow inline functions
// in header files to use these macros.
void arch_panic(void);
void backtrace(void);

void printk(const char *fmt, ...);

// TODO: Add ifdef guard "CONFIG_DISABLE_ANSI_COLOR"
#define COLOR_BOLD_RED "\e[1;91m"
#define COLOR_YELLOW "\e[0;33m"
#define COLOR_CYAN "\e[0;96m"
#define COLOR_RESET "\e[0m"

#ifdef RELEASE_BUILD
#    define TRACE(fmt, ...)
#    define TRACE(fmt, ...)
#else
#    define TRACE(fmt, ...) \
        printk("[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#    define DEBUG(fmt, ...) \
        printk(COLOR_CYAN "[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
#endif

#define INFO(fmt, ...) printk("[kernel] " fmt COLOR_RESET "\n", ##__VA_ARGS__)
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
    PANIC("[kernel] not yet implemented: %s:%d", __FILE__, __LINE__)

#define ASSERT(expr)                                           \
    do {                                                       \
        if (!(expr)) {                                         \
            printk("[kernel] ASSERTION FAILURE: " #expr "\n"); \
            backtrace();                                       \
            arch_panic();                                      \
        }                                                      \
    } while (0)

#endif
