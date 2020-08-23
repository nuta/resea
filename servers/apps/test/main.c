#include <config.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include "test.h"

int failed = 0;

static void exit_emulator(void) {
    sys_kdebug("q", 1, NULL, 0);
}

void main(void) {
    INFO("starting integrated tests...");

    ipc_test();
    libcommon_test();
    libresea_test();

    if (failed) {
        WARN("Failed %d tests", failed);
    } else {
        INFO("Passed all tests!");
    }

    exit_emulator();
}
