#include <resea.h>
#include <resea_idl.h>

static cid_t memmgr_ch = 1;
static cid_t kernel_ch = 2;

void ipc_test(void) {
    error_t err;

    INFO(">>> IPC tests (inline only)");
    struct message *m = get_ipc_buffer();
    m->header = MEMMGR_BENCHMARK_NOP_HEADER;
    err = sys_ipc(memmgr_ch, IPC_SEND | IPC_RECV);
    if (err != OK) {
        WARN("sys_ipc returned an error: %d", err);
    }

    INFO(">>> IPC tests (w/ page payloads)");
    uintptr_t page;
    size_t page_num;
    TRY_OR_PANIC(call_memmgr_alloc_pages(memmgr_ch, 0, valloc(128), &page,
                                         &page_num));
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

void poweroff(void) {
    TRY_OR_PANIC(call_io_allow_iomapped_io(kernel_ch));
    INFO("Power off.");

    // Try a QEMU-specific port to exit the emulator.
    __asm__ __volatile__("outw %%ax, %%dx" :: "a"(0x2000), "d"(0x604));

    // Wait for a while.
    for(volatile int i = 0; i < 0x10000; i++);

    ERROR("Failed to power off.");

    // Unreachable for loop to supress a compiler warning.
    for(;;);
}

void main(void) {
    INFO("*");
    INFO("* Kernel test");
    INFO("*");

    ipc_test();
    float_test();

    INFO("Finished all tests!");
    poweroff();
}
