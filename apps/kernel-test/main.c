#include <resea.h>
#include <resea_idl.h>


void ipc_test(void) {
    error_t err;
    cid_t memmgr = 1;

    INFO(">>> IPC tests (inline only)");
    struct benchmark_nop_msg *m = (struct benchmark_nop_msg *) get_ipc_buffer();
    m->header = BENCHMARK_NOP_HEADER;
    err = sys_ipc(memmgr, IPC_SEND | IPC_RECV);
    if (err != OK) {
        WARN("sys_ipc returned an error: %d", err);
    }

    INFO(">>> IPC tests (w/ page payloads)");
    uintptr_t page;
    size_t page_num;
    err = alloc_pages(memmgr, 0, valloc(128), &page, &page_num);
    if (err != OK) {
        WARN("alloc_pages returned an error: %d", err);
    }
    uint32_t *p = (uint32_t *) PAGE_PAYLOAD_ADDR(page);
    INFO("received a page: %p, addr=%p", page, p);
    *p = 0xbeefbeef;
    INFO("the received page is accessible");
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
