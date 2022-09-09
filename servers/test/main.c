#include <print_macros.h>
#include <resea/ipc.h>
#include <resea/test.h>
#include <string.h>

void main(void) {
    TRACE("ready");
    INFO("Done!");
}

UNIT_TEST("hello") {
    INFO("Hello, world from unittest!");
    EXPECT_EQ(1, 1);
    EXPECT_TRUE(true);
    EXPECT_EQ(1, 2);
}
