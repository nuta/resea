#include "test.h"
#include <resea/malloc.h>
#include <resea/printf.h>

void libresea_test(void) {
    // malloc
    void *ptr;
    ptr = malloc(128);
    TEST_ASSERT(ptr != NULL);
    free(ptr);
    ptr = malloc(1);
    TEST_ASSERT(ptr != NULL);
    free(ptr);
}
