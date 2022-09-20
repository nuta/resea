#include <print.h>
#include <resea/ipc.h>
#include <resea/test.h>
#include <string.h>

void main(void) {
    INFO("Done!");
}

UNIT_TEST("hello") {
    INFO("Hello, world from unittest!");
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
}
