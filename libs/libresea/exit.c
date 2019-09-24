#include <resea.h>
#include <resea_idl.h>

NORETURN void exit(int status) {
    call_runtime_exit_current(1, status);
    WARN("Error: runtime.exit_current returned");
    backtrace();
    unreachable();
}
