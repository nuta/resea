#include <unittest.h>
#include <resea/printf.h>


void exit(int);
void printf(const char *fmt, ...);
void fflush(void *);

void unittest_main(void);

static bool failed = false;
static unsigned tests_passed = 0;
static unsigned tests_failed = 0;

extern void (*unittest_funcs[])(void);
extern const char *unittest_titles[];

static void run_tests(void) {
    for (unsigned int i = 0; unittest_funcs[i]; i++) {
        printf("[unittest] %s ... ", unittest_titles[i]);
        fflush(0);
        unittest_funcs[i]();
        if (failed) {
            tests_failed++;
        } else {
            printf("\x1b[32mok\x1b[0m\n");
            tests_passed++;
        }
    }
}

static void on_fail(void) {
    if (!failed) {
        failed = true;
        printf("\x1b[1;91mfail!\x1b[0m\n");
    }
}

void unittest_aborted(void) {
    on_fail();
}

void unittest_expect_fail(const char *file, unsigned lineno, const char *expected,
                          unsigned long long a, unsigned long long b) {
    on_fail();
    printf("\n\x1b[33mAssertion Failure at %s:%d:\n\n\t%s\x1b[0m\n\n"
        "\t\x1b[91mleft: %#llx\x1b[0m, \x1b[92mright: %#llx\x1b[0m\n\n",
        file, lineno, expected, a, b);
}

void report_test_results(void) {
    if (tests_failed > 0) {
        printf("[unittest] \x1b[1;91mFailed %d/%d tests!\x1b[0m\n",
            tests_failed, tests_passed + tests_failed);
    } else {
        printf("[unittest] \x1b[32mPassed all %d tests!\x1b[0m\n", tests_passed);
    }
}

/// The entry point of the unit testing executable. It uses the "constructor"
/// attribute not to run main() function in the test target.
__attribute__((constructor)) void unittest_main(void) {
    printf("[unittest] Running unit tests in %s\n", UNITTEST_TARGET_NAME);
    run_tests();
    report_test_results();
    exit(0);
}

/// In libs, main() is not defined.
__weak void main(void) {
    unittest_main();
}
