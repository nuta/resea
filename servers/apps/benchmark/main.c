#include <config.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <string.h>
#define NUM_ITERS 128

#ifdef __x86_64__
typedef uint64_t cycles_t;
static inline cycles_t cycle_counter(void) {
    uint32_t eax, edx;
    __asm__ __volatile__("rdtscp" : "=a"(eax), "=d"(edx) :: "%ecx");
    return (((cycles_t) edx) << 32) | eax;
}

#elif __aarch64__
typedef uint64_t cycles_t;
static inline cycles_t cycle_counter(void) {
    uint64_t value;
    __asm__ __volatile__("mrs %0, pmccntr_el0" : "=r" (value));
    return value;
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
    METRIC(name, min);
}

void main(void) {
    cycles_t iters[NUM_ITERS];
    INFO("starting IPC benchmark...");

    for (int i = 0; i < NUM_ITERS; i++) {
        cycles_t start = cycle_counter();
        iters[i] = cycle_counter() - start;
    }
    print_stats("reading cycle counter", iters, NUM_ITERS);

    uint8_t tmp1[512], tmp2[512];
    for (int i = 0; i < NUM_ITERS; i++) {
        cycles_t start = cycle_counter();
        memcpy(tmp1, tmp2, 8);
        iters[i] = cycle_counter() - start;
    }
    print_stats("memcpy (8-bytes)", iters, NUM_ITERS);

    for (int i = 0; i < NUM_ITERS; i++) {
        cycles_t start = cycle_counter();
        memcpy(tmp1, tmp2, 64);
        iters[i] = cycle_counter() - start;
    }
    print_stats("memcpy (64-bytes)", iters, NUM_ITERS);

    for (int i = 0; i < NUM_ITERS; i++) {
        cycles_t start = cycle_counter();
        memcpy(tmp1, tmp2, 512);
        iters[i] = cycle_counter() - start;
    }
    print_stats("memcpy (512-bytes)", iters, NUM_ITERS);

    for (int i = 0; i < NUM_ITERS; i++) {
        cycles_t start = cycle_counter();
        syscall(SYS_NOP, 0, 0, 0, 0, 0);
        iters[i] = cycle_counter() - start;
    }
    print_stats("nop syscall", iters, NUM_ITERS);

    //
    //  IPC round-trip benchmark
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        struct message m = { .type = NOP_MSG };
        cycles_t start = cycle_counter();
        ipc_call(INIT_TASK, &m);
        iters[i] = cycle_counter() - start;
    }
    print_stats("IPC round-trip (simple)", iters, NUM_ITERS);

    //
    //  IPC round-trip benchmark (with small ool payload)
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        // Since we don't access the data (ool_payload) to be sent, the kernel
        // internally handles the page fault on the first message passing. Thus
        // it should take signficantly long on the first time.
        static char ool_payload[1] = "A";

        struct message m;
        m.type = NOP_WITH_OOL_MSG;
        m.nop_with_ool.data = ool_payload;
        m.nop_with_ool.data_len = PAGE_SIZE;

        cycles_t start = cycle_counter();
        ipc_call(INIT_TASK, &m);
        iters[i] = cycle_counter() - start;
        ASSERT(m.type == NOP_WITH_OOL_REPLY_MSG);
        free((void *) m.nop_with_ool_reply.data);
    }
    print_stats("IPC round-trip (with 1-byte ool)", iters, NUM_ITERS);

    //
    //  IPC round-trip benchmark (with ool payload)
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        // Since we don't access the data (ool_payload) to be sent, the kernel
        // internally handles the page fault on the first message passing. Thus
        // it should take signficantly long on the first time.
        static char ool_payload[PAGE_SIZE] = "This is a ool payload!";

        struct message m;
        m.type = NOP_WITH_OOL_MSG;
        m.nop_with_ool.data = ool_payload;
        m.nop_with_ool.data_len = PAGE_SIZE;

        cycles_t start = cycle_counter();
        ipc_call(INIT_TASK, &m);
        iters[i] = cycle_counter() - start;
        ASSERT(m.type == NOP_WITH_OOL_REPLY_MSG);
        free((void *) m.nop_with_ool_reply.data);
    }
    print_stats("IPC round-trip (with PAGE_SIZE-sized ool)", iters, NUM_ITERS);

}
