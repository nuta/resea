#include <print_macros.h>
#include <resea/malloc.h>
#include <resea/task.h>

void main(void);

__noreturn void resea_init(void) {
    malloc_init();
    main();
    // TODO:
    // task_exit();
    UNREACHABLE();
}
