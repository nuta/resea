#include <std/async.h>
#include <std/malloc.h>

void std_init(void) {
    malloc_init();
    async_init();
}
