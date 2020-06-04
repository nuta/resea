#include <config.h>
#include <resea/printf.h>
#include <resea/io.h>
#include "test.h"

int failed = 0;

static void exit_emulator(void) {
#ifdef CONFIG_ARCH_X64
    // QEMU
    io_out16(0x604, 0x2000);
#endif
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
