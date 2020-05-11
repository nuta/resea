#include <message.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#define NUM_ITERS 128

#ifdef __x86_64__
typedef uint64_t cycles_t;
static inline cycles_t cycle_counter(void) {
    uint32_t eax, edx;
    __asm__ __volatile__("rdtscp" : "=a"(eax), "=d"(edx) :: "%ecx");
    return (((cycles_t) edx) << 32) | eax;
}
#else
#    error "unsupported arch"
#endif

static void print_stats(const char *name, cycles_t *iters, size_t num_iters) {
    cycles_t avg = 0, min = 0xffffffffffffffff, max = 0;
    for (size_t i = 0; i < num_iters; i++) {
        min = MIN(min, iters[i]);
        max = MAX(max, iters[i]);
        avg += iters[i];
    }

    avg /= NUM_ITERS;
    INFO("%s: avg=%d, min=%d, max=%d", name, avg, min, max);
}

void main(void) {
    INFO("starting IPC benchmark...");

    //
    //  IPC round-trip benchmark
    //
    cycles_t iters[NUM_ITERS];
    for (int i = 0; i < NUM_ITERS; i++) {
        struct message m = { .type = NOP_MSG };
        cycles_t start = cycle_counter();
        ipc_call(INIT_TASK_TID, &m);
        iters[i] = cycle_counter() - start;
    }

    print_stats("IPC round-trip", iters, NUM_ITERS);
}
