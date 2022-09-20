#include <print.h>
#include <resea/malloc.h>
#include <resea/task.h>
#include <resea/test.h>

void main(void);

extern char __unit_tests[];
extern char __unit_tests_end[];

static void run_unit_tests(void) {
    struct __unit_test *test = (struct __unit_test *) __unit_tests;

    unsigned n = 0;
    while ((uintptr_t) test < (uintptr_t) __unit_tests_end) {
        printf("test %s ... ", test->name);
        printf_flush();
        test->fn();
        printf("\x1b[92mok\x1b[0m\n");
        test++;
        n++;
    }

    if (n > 0) {
        INFO("all %d unit tests passed", n);
    }
}

__noreturn void resea_init(void) {
    malloc_init();

    run_unit_tests();

    main();
    task_exit();
}

void panic_lock(void) {
}

__noreturn void halt(void) {
    task_exit();
}
