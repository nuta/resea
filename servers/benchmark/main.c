#include <resea.h>
#include <idl_stubs.h>

#define NUM_ITERS 1000

static inline uint64_t readcyclecounter(void) {
#ifdef __x86_64__
    uint32_t low, high;
    __asm__ __volatile__("rdtscp" : "=a"(low), "=d"(high)::"%ecx");
    return ((uint64_t) high << 32) | low;
#else
#    error "unsupported CPU architecture"
#endif
}

int main(void) {
    INFO("*");
    INFO("* Benchmark");
    INFO("*");

    INFO("IPC round-trip overhead");
    cid_t memmgr = 1;
    uint32_t ops = IPC_SEND | IPC_RECV;
    for (int i = 0; i < NUM_ITERS; i++) {
        get_ipc_buffer()->header = MEMMGR_BENCHMARK_NOP_HEADER;
        uint64_t start = readcyclecounter();
        error_t err = sys_ipc(memmgr, ops);
        uint64_t end = readcyclecounter();
        if (err != OK) {
            WARN("sys_ipc returned an error: %d", err);
        }
        INFO("Iter #%d: %d cycles", i + 1, end - start);
    }

    INFO("Finished all benchmarks!");
    return 0;
}
