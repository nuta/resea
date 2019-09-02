#include <resea.h>
#include <resea_idl.h>


void ipc_test(void) {
    INFO(">>> IPC tests");
    struct benchmark_nop_msg *m = (struct benchmark_nop_msg *) get_ipc_buffer();
    m->header = BENCHMARK_NOP_HEADER;
    error_t err = sys_ipc(1 /* memmgr */, IPC_SEND | IPC_RECV);
    if (err != OK) {
        WARN("sys_ipc returned an error: %d", err);
    }
}

void float_test(void) {
    // TODO: verify that floating-point registers are saved and restored between
    // context switches.
    INFO(">>> Float tests");

    // Do a completely pointless floating-point calculation to invoke lazy FPU
    // context switching.
    volatile double a = __builtin_readcyclecounter() / 12.345;
    volatile double b = 9.87;

    for (int i = 0; i < 4; i++) {
        a *= b;
    }

    assert(a != __builtin_inf());
}

int main(void) {
    INFO("*");
    INFO("* Kernel test");
    INFO("*");

    ipc_test();
    float_test();

    INFO("Finished all tests!");
    exit_kernel_test(1 /* memmgr */);

    return 0;
}
