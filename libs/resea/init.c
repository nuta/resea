#include <resea/malloc.h>

__noreturn void resea_init(void) {
    malloc_init();
    main();
    // FIXME:
    UNREACHABLE();
    // task_exit();
}
