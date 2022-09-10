#include <print_macros.h>
#include <resea/malloc.h>
#include <resea/task.h>
#include <resea/test.h>

void main(void);

extern char __unit_tests[];
extern char __unit_tests_end[];

static void run_unit_tests(void) {
    struct __unit_test *test = (struct __unit_test *) __unit_tests;
    while ((uintptr_t) test < (uintptr_t) __unit_tests_end) {
        INFO("Running unit test: %s", test->name);
        test->fn();
        test++;
    }
}

__noreturn void resea_init(void) {
    malloc_init();

    // FIXME:
    // run_unit_tests();

    main();
    task_exit();
}

void panic_lock(void) {
}

__noreturn void halt(void) {
    task_exit();
}
