#include <resea/mymalloc.h>
#include <resea/printf.h>
#include <string.h>
#include "test.h"
#define NUM_PTRS 16

void malloc_test(void) {
    my_malloc_init();
    void *ptr[NUM_PTRS];
    memset(ptr, 0, sizeof(ptr));
    TEST_ASSERT(
        1 != 2);  // Sample test just to see if this test function is called
    int sz = 1;

    for (size_t i = 0; i < NUM_PTRS - 1; i++) {
        ptr[i] = my_malloc(sz << i);
        TEST_ASSERT(ptr[i] != NULL);
    }

    // for (size_t i = 0; i < 4; i++) {
    //     ptr[i] = my_malloc(1 << (2 * i));
    //     TEST_ASSERT(ptr[i] != NULL);
    // }
    ptr[NUM_PTRS - 1] = my_malloc((1 << 15) + 8);
    // TEST_ASSERT(ptr[4] != NULL);
    for (size_t i = 0; i < NUM_PTRS; i++) {
        my_free(ptr[i]);
        // TEST_ASSERT(ptr[i] == NULL);
    }
    for (size_t i = 0; i < NUM_PTRS - 1; i++) {
        ptr[i] = my_malloc(sz << i);
        TEST_ASSERT(ptr[i] != NULL);
    }
    ptr[NUM_PTRS - 1] = my_malloc((1 << 15) + 8);
    for (size_t i = 0; i < NUM_PTRS; i++) {
        my_free(ptr[i]);
        // TEST_ASSERT(ptr[i] == NULL);
    }
}