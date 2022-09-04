#include <print_macros.h>
#include <resea/malloc.h>
#include <resea/task.h>

void main(void);
#include <resea/syscall.h>  //FIXME:

__noreturn void resea_init(void) {
    sys_console_write("hello!\n", 7);
    malloc_init();
    main();
    // TODO:
    // task_exit();
    UNREACHABLE();
}
