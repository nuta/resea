#include <resea/mymalloc.h>
#include <resea/printf.h>
#include "test.h"

void malloc_test(void) {
    my_malloc_init();
    void *ptr[5] = {NULL};
    TEST_ASSERT(1 != 2);
    int sz = 1 << 7;
    for (size_t i = 0; i < 4; i++) {
        ptr[i] = my_malloc(sz << i);
        TEST_ASSERT(ptr[i] != NULL);
    }

    // for (size_t i = 0; i < 4; i++) {
    //     ptr[i] = my_malloc(1 << (2 * i));
    //     TEST_ASSERT(ptr[i] != NULL);
    // }
    ptr[4] = my_malloc((1 << 15) + 8);
    // TEST_ASSERT(ptr[4] != NULL);
    for (size_t i = 0; i < 5; i++) {
        my_free(ptr[i]);
        // TEST_ASSERT(ptr[i] == NULL);
    }
    for (size_t i = 0; i < 4; i++) {
        ptr[i] = my_malloc(sz << i);
        TEST_ASSERT(ptr[i] != NULL);
    }
    ptr[4] = my_malloc((1 << 15) + 8);
    for (size_t i = 0; i < 5; i++) {
        my_free(ptr[i]);
        // TEST_ASSERT(ptr[i] == NULL);
    }
}