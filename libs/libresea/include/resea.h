#ifndef __RESEA_H__
#define __RESEA_H__

#include <types.h>
#include <color.h>

#include <resea/ipc.h>

//
//  Arch-dependent stuff.
//
#ifdef __x86_64__
NORETURN void unreachable(void);

struct stack_frame {
    struct stack_frame *next;
    uint64_t return_addr;
} PACKED;

static inline struct stack_frame *get_stack_frame(void) {
    return (struct stack_frame *) __builtin_frame_address(0);
}
#else
#    error "unsupported CPU architecture"
#endif

//
//  Misc.
//
void putchar(char ch);
void puts(const char *s);
void printf(const char *fmt, ...);
void backtrace(void);
void exit(int status);
void try_or_panic(error_t err, const char *file, int lineno);

const char *__program_name(void);
#define PROGRAM_NAME  __program_name()

#ifdef RELEASE_BUILD
#    define TRACE(fmt, ...)
#    define TRACE(fmt, ...)
#else
#    define TRACE(fmt, ...) \
    printf("[%s] " fmt COLOR_RESET "\n", PROGRAM_NAME, ##__VA_ARGS__)
#    define DEBUG(fmt, ...) \
    printf(COLOR_CYAN "[%s] " fmt COLOR_RESET "\n", PROGRAM_NAME, ##__VA_ARGS__)
#endif

#define INFO(fmt, ...) \
    printf(COLOR_BLUE "[%s] " fmt COLOR_RESET "\n", PROGRAM_NAME, ##__VA_ARGS__)
#define WARN(fmt, ...)                                                         \
    printf(COLOR_YELLOW "[%s] WARN: " fmt COLOR_RESET "\n", PROGRAM_NAME,      \
        ##__VA_ARGS__)
#define OOPS(fmt, ...)                                                         \
    do {                                                                       \
        printf(COLOR_YELLOW "[%s] Oops: " fmt COLOR_RESET "\n", PROGRAM_NAME,  \
            ##__VA_ARGS__);                                                    \
        backtrace();                                                           \
    } while (0)

#define BUG(fmt, ...)                                                          \
    do {                                                                       \
        printf(COLOR_BOLD_RED "[%s] BUG: " fmt COLOR_RESET "\n", PROGRAM_NAME, \
            ##__VA_ARGS__);                                                    \
        backtrace();                                                           \
        exit(1);                                                               \
    } while (0)

#define ERROR(fmt, ...)                                                        \
    do {                                                                       \
        printf(COLOR_BOLD_RED "[%s] Error: " fmt COLOR_RESET "\n",             \
               PROGRAM_NAME, ##__VA_ARGS__);                                   \
        backtrace();                                                           \
        exit(1);                                                               \
    } while (0)

#define UNIMPLEMENTED() ERROR("not yet implemented: %s:%d", __FILE__, __LINE__)

#define TRY_OR_PANIC(err) try_or_panic(err, __FILE__, __LINE__)
#define TRY(expr) do {                                                         \
        error_t __err = expr;                                                  \
        if (__err != OK) {                                                     \
            return __err;                                                      \
        }                                                                      \
    } while (0)
#define TRY_OR_OOPS(expr) do {                                                 \
        error_t __err = expr;                                                  \
        if (__err != OK) {                                                     \
            OOPS("An error occurred: %vE", __err);                             \
            return __err;                                                      \
        }                                                                      \
    } while (0)

#define assert(expr)                                                           \
    do {                                                                       \
        if (!(expr)) {                                                         \
            printf(COLOR_BOLD_RED "[%s] ASSERTION FAILURE: " #expr "\n"        \
                   COLOR_RESET, PROGRAM_NAME);                                 \
            backtrace();                                                       \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

#endif
