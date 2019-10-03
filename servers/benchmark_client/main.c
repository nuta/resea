#include <resea.h>
#include <server.h>
#include <idl_stubs.h>

#define WARM_UP_ITERS 32
#define NUM_ITERS 256

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

struct benchmark {
    const char *name;
    uint64_t cycles[NUM_ITERS];
};

static inline void bench_start(struct benchmark *bench, const char *name) {
    bench->name = name;
}

static inline void bench_end(struct benchmark *bench) {
    uint64_t sum = 0;
    STATIC_ASSERT(NUM_ITERS > 0);
    uint64_t lowest = bench->cycles[0];
    uint64_t highest = bench->cycles[0];
    for (int i = WARM_UP_ITERS; i < NUM_ITERS; i++) {
        sum += bench->cycles[i];
        lowest = MIN(lowest, bench->cycles[i]);
        highest = MAX(highest, bench->cycles[i]);
    }

    uint64_t avg = sum / (NUM_ITERS - WARM_UP_ITERS);

    INFO("%s: %d cycles (+- %d)",
        bench->name, avg, MAX(avg - lowest, highest - avg));
}

static inline void bench_start_iter(struct benchmark *bench, int i) {
    uint64_t current = readcyclecounter();
    bench->cycles[i] = current;
}

static inline void bench_end_iter(struct benchmark *bench, int i) {
    uint64_t current = readcyclecounter();
    bench->cycles[i] = current - bench->cycles[i];
}

int main(void) {
    INFO("*");
    INFO("* Benchmark");
    INFO("*");

    INFO(">>> Connecting to the benchmark server...");
    cid_t benchmark_server;
    TRY_OR_PANIC(server_connect(memmgr_ch, BENCHMARK_INTERFACE,
                                &benchmark_server));

    INFO(">>> Running benchmarks (results are average # of CPU cyles)...");
    struct benchmark bench;

    bench_start(&bench, "system call overhead");
    for (int i = 0; i < NUM_ITERS; i++) {
        bench_start_iter(&bench, i);
        sys_nop();
        bench_end_iter(&bench, i);
    }
    bench_end(&bench);

    bench_start(&bench, "open system call");
    for (int i = 0; i < NUM_ITERS; i++) {
        bench_start_iter(&bench, i);
        int cid_or_err = sys_open();
        bench_end_iter(&bench, i);
        if (cid_or_err < 0) {
            ERROR("sys_open returned an error: %d", -cid_or_err);
        }
    }
    bench_end(&bench);

    uint32_t ops = IPC_SEND | IPC_RECV;
    struct message *m = get_ipc_buffer();
    bench_start(&bench, "IPC round-trip overhead");
    for (int i = 0; i < NUM_ITERS; i++) {
        m->header = BENCHMARK_NOP_MSG;
        bench_start_iter(&bench, i);
        error_t err = sys_ipc(benchmark_server, ops);
        bench_end_iter(&bench, i);
        if (err != OK) {
            ERROR("sys_ipc returned an error: %d", err);
        }
    }
    bench_end(&bench);

    INFO(">>> Finished all benchmarks!");
    for (;;);
    return 0;
}
