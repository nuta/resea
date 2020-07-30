#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
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
    cycles_t iters[NUM_ITERS];
    INFO("starting IPC benchmark...");

    //
    //  IPC round-trip benchmark
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        struct message m = { .type = NOP_MSG };
        cycles_t start = cycle_counter();
        ipc_call(INIT_TASK, &m);
        iters[i] = cycle_counter() - start;
    }
    print_stats("IPC round-trip", iters, NUM_ITERS);

    //
    //  IPC round-trip benchmark (with small bulk payload)
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        // Since we don't access the data (bulk_payload) to be sent, the kernel
        // internally handles the page fault on the first message passing. Thus
        // it should take signficantly long on the first time.
        static char bulk_payload[1] = "A";

        struct message m;
        m.type = NOP_WITH_BULK_MSG;
        m.nop_with_bulk.data = bulk_payload;
        m.nop_with_bulk.data_len = PAGE_SIZE;

        cycles_t start = cycle_counter();
        ipc_call(INIT_TASK, &m);
        iters[i] = cycle_counter() - start;
        ASSERT(m.type == NOP_WITH_BULK_REPLY_MSG);
        free((void *) m.nop_with_bulk_reply.data);
    }
    print_stats("IPC round-trip (with 1-byte bulk)", iters, NUM_ITERS);

    //
    //  IPC round-trip benchmark (with bulk payload)
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        // Since we don't access the data (bulk_payload) to be sent, the kernel
        // internally handles the page fault on the first message passing. Thus
        // it should take signficantly long on the first time.
        static char bulk_payload[PAGE_SIZE] = "This is a bulk payload!";

        struct message m;
        m.type = NOP_WITH_BULK_MSG;
        m.nop_with_bulk.data = bulk_payload;
        m.nop_with_bulk.data_len = PAGE_SIZE;

        cycles_t start = cycle_counter();
        ipc_call(INIT_TASK, &m);
        iters[i] = cycle_counter() - start;
        ASSERT(m.type == NOP_WITH_BULK_REPLY_MSG);
        free((void *) m.nop_with_bulk_reply.data);
    }
    print_stats("IPC round-trip (with PAGE_SIZE-sized bulk)", iters, NUM_ITERS);

}
