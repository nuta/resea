#pragma once

struct __unit_test {
    const char *name;
    void (*fn)(void);
};

#define ___CONCAT(x, y) x##y
#define __CONCAT(x, y)  ___CONCAT(x, y)

#ifdef TEST

#    define UNIT_TEST(__name)                                                  \
        void __CONCAT(__unit_test_fn_, __LINE__)(void);                        \
        static struct __unit_test __CONCAT(__unit_test_, __LINE__)             \
            __attribute__((section(".unit_tests")))                            \
            __attribute__((__used__)) = {                                      \
                .name = (__name), .fn = __CONCAT(__unit_test_fn_, __LINE__)};  \
        void __CONCAT(__unit_test_fn_, __LINE__)(void)

#else

#    define UNIT_TEST(__name)                                                  \
        __unused static void __CONCAT(__unit_test_fn_, __LINE__)(void)

#endif

#define EXPECT_TRUE(expr)                                                      \
    do {                                                                       \
        __typeof__(expr) _expr = (expr);                                       \
        if (!_expr) {                                                          \
            PANIC("%s:%d: expected, '" #expr "' to be true", __FILE__,         \
                  __LINE__);                                                   \
        }                                                                      \
    } while (0)

#define EXPECT_EQ(expected, actual)                                            \
    do {                                                                       \
        __typeof__(expected) _expected = (expected);                           \
        __typeof__(actual) _actual = (actual);                                 \
        if (_expected != _actual) {                                            \
            PANIC("%s:%d: expected %d, but got %d", __FILE__, __LINE__,        \
                  (_expected), (_actual));                                     \
        }                                                                      \
    } while (0)
