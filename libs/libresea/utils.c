#include <resea.h>

void try_or_panic(error_t err, const char *file, int lineno) {
    if (err != OK) {
        ERROR("%s:%d: an unexpected error occurred.", file, lineno);
        backtrace();
        exit(1);
    }
}
