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

int main(void) {
    INFO("*");
    INFO("* Kernel test");
    INFO("*");

    ipc_test();

    INFO("Finished all tests!");
    exit_kernel_test(1 /* memmgr */);

    return 0;
}
