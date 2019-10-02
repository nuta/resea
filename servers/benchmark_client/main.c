#include <resea.h>
#include <server.h>
#include <idl_stubs.h>

#define NUM_ITERS 1000

static cid_t memmgr_ch = 1;

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

    INFO(">>> Connecting to the benchmark server...");
    cid_t benchmark_server;
    TRY_OR_PANIC(server_connect(memmgr_ch, BENCHMARK_INTERFACE,
                                &benchmark_server));


    INFO(">>> IPC round-trip overhead");
    uint32_t ops = IPC_SEND | IPC_RECV;
    uint64_t cycles[NUM_ITERS];
    struct message *m = get_ipc_buffer();
    for (int i = 0; i < NUM_ITERS; i++) {
        m->header = BENCHMARK_NOP_HEADER;

        uint64_t start = readcyclecounter();
        error_t err = sys_ipc(benchmark_server, ops);
        uint64_t end = readcyclecounter();

        if (err != OK) {
            ERROR("sys_ipc returned an error: %d", err);
        }

        cycles[i] = end - start;
    }

    uint64_t sum = 0;
    STATIC_ASSERT(NUM_ITERS > 0);
    uint64_t lowest = cycles[0];
    uint64_t highest = cycles[0];
    for (int i = 0; i < NUM_ITERS; i++) {
        sum += cycles[i];
        lowest = MIN(lowest, cycles[i]);
        highest = MAX(highest, cycles[i]);
    }

    uint64_t avg = sum / NUM_ITERS;

    INFO("Finished all benchmarks!");
    INFO("IPC overhead (nop): %d cycles +- %d",
         avg, MAX(avg - lowest, highest - avg));
    return 0;
}
