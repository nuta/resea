#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include "test.h"
#define NUM_PTRS 16

void malloc_test(void) {
    malloc_init();
    void *ptr[NUM_PTRS];
    memset(ptr, 0, sizeof(ptr));
    int sz = 1;

    // Allocate chunks of different sizes
    for (size_t i = 0; i < NUM_PTRS - 1; i++) {
        ptr[i] = malloc(sz << i);
        TEST_ASSERT(ptr[i] != NULL);
    }

    // Chunk corresponding to largest bin
    ptr[NUM_PTRS - 1] = malloc((1 << 15) + 8);
    for (size_t i = 0; i < NUM_PTRS; i++) {
        free(ptr[i]);
    }

    // Do the same allocation again and see if an existing chunk is allocated
    for (size_t i = 0; i < NUM_PTRS - 1; i++) {
        ptr[i] = malloc(sz << i);
        TEST_ASSERT(ptr[i] != NULL);
    }

    ptr[NUM_PTRS - 1] = malloc((1 << 15) + 8);
    for (size_t i = 0; i < NUM_PTRS; i++) {
        free(ptr[i]);
    }
}
