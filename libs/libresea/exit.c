#include <resea.h>
#include <resea_idl.h>

NORETURN void exit(int code) {
    call_runtime_exit_current(1, code);
    WARN("Error: runtime.exit_current returned");
    backtrace();
    unreachable();
}
