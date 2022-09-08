#pragma once

struct __unit_test {
    const char *name;
    void (*fn)(void);
};

#define ___CONCAT(x, y) x##y
#define __CONCAT(x, y)  ___CONCAT(x, y)

#define UNIT_TEST(__name)                                                      \
    void __CONCAT(__unit_test_fn_, __LINE__)(void);                            \
    static struct __unit_test __CONCAT(__unit_test_, __LINE__)                 \
        __attribute__((section(".unit_tests"))) __attribute__((__used__)) = {  \
            .name = (__name), .fn = __CONCAT(__unit_test_fn_, __LINE__)};      \
    void __CONCAT(__unit_test_fn_, __LINE__)(void)

#define EXPECT_EQ(__expected, __actual)                                        \
    do {                                                                       \
        __typeof__(__expected) __expected__ = (__expected);                    \
        __typeof__(__actual) __actual__ = (__actual);                          \
        if (__expected__ != __actual__) {                                      \
            PANIC("%s:%d: expected %d, but got %d", __FILE__, __LINE__,        \
                  (__expected__), (__actual__));                               \
        }                                                                      \
    } while (0)
