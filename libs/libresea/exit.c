#include <resea.h>
#include <idl_stubs.h>

NORETURN void exit(int status) {
    OOPS("Killing the current process...");
    call_runtime_exit_current(1, status);
    WARN("Error: runtime.exit_current returned");
    backtrace();
    unreachable();
}
