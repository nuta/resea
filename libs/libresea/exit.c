#include <resea.h>
#include <resea_idl.h>

NORETURN void exit(int code) {
    call_runtime_exit_current(1, code);
    printf("[libresea] Error: exit.exit_current returned\n");
    do_backtrace("[libresea] ");
    unreachable();
}
