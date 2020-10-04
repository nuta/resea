#ifndef __UNITTEST_H__
#define __UNITTEST_H__

void unittest_aborted(void);
void unittest_expect_fail(const char *file, unsigned lineno, const char *expected,
                          unsigned long long a, unsigned long long b);

#define ___CAT(a, b) a##b
#define __CAT(a, b) ___CAT(a, b)

#define TEST(title)                                                            \
    const char *__CAT(__unittest_title, __COUNTER__) =                         \
        __FILE_NAME__ ": " title;                                              \
    void __CAT(__unittest_, __COUNTER__)(void)                                 \

// TODO: Use _Genric to determine the type of `a` and `b` and print `a` and `b`
//        properly.
#define _TEST_EXPECT(a, b, cmp)                                                \
    if (!((a) cmp (b))) {                                                      \
        unittest_expect_fail(__FILE__, __LINE__, #a " " #cmp " " #b, a, b);    \
    }

#define TEST_EXPECT_EQ(a, b) _TEST_EXPECT(a, b, ==)
#define TEST_EXPECT_NE(a, b) _TEST_EXPECT(a, b, !=)
#define TEST_EXPECT_LT(a, b) _TEST_EXPECT(a, b, <)
#define TEST_EXPECT_LE(a, b) _TEST_EXPECT(a, b, <=)
#define TEST_EXPECT_GT(a, b) _TEST_EXPECT(a, b, >)
#define TEST_EXPECT_GE(a, b) _TEST_EXPECT(a, b, >=)

#endif
