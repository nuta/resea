#include "test.h"
#include <resea/printf.h>
#include <string.h>

void libcommon_test(void) {
    TEST_ASSERT(!memcmp("a", "a", 1));
    TEST_ASSERT(!memcmp("a", "b", 0));
    TEST_ASSERT(memcmp("ab", "aa", 2) != 0);

    TEST_ASSERT(!strncmp("a", "a", 1));
    TEST_ASSERT(!strncmp("a", "b", 0));
}
