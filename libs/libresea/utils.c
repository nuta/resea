#include <resea.h>

void try_or_panic(error_t err, const char *file, int lineno) {
    if (err != OK) {
        printf("Error: %s:%d: an unexpected error occurred.\n", file, lineno);
        do_backtrace("");
        exit(1);
    }
}
