#include <resea.h>

NORETURN void panic_at(const char *file, int lineno) {
    ERROR("%s:%d: an unexpected error occurred.", file, lineno);
    backtrace();
    exit(1);
}
