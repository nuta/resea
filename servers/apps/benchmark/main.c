#include <config.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <string.h>

#ifdef __x86_64__
static inline uint64_t cycle_counter(void) {
    uint32_t eax, edx;
    __asm__ __volatile__("rdtscp" : "=a"(eax), "=d"(edx)::"%ecx");
    return (((uint64_t) edx) << 32) | eax;
}

static inline uint64_t l1d_cache_counter(void) {
    // TODO:
    return 0;
}

static inline uint64_t l2d_cache_counter(void) {
    // TODO:
    return 0;
}

static inline uint64_t mem_access_counter(void) {
    // TODO:
    return 0;
}

static inline uint64_t exception_counter(void) {
    // TODO:
    return 0;
}

static inline uint64_t l1_tlb_refill_counter(void) {
    // TODO:
    return 0;
}
#elif __aarch64__
static inline uint64_t cycle_counter(void) {
    uint64_t value;
    __asm__ __volatile__("mrs %0, pmccntr_el0" : "=r"(value));
    return value;
}

static inline uint64_t l1d_cache_counter(void) {
    uint64_t value;
    __asm__ __volatile__("mrs %0, pmevcntr0_el0" : "=r"(value));
    return value;
}

static inline uint64_t l2d_cache_counter(void) {
    uint64_t value;
    __asm__ __volatile__("mrs %0, pmevcntr1_el0" : "=r"(value));
    return value;
}

static inline uint64_t mem_access_counter(void) {
    uint64_t value;
    __asm__ __volatile__("mrs %0, pmevcntr2_el0" : "=r"(value));
    return value;
}

static inline uint64_t exception_counter(void) {
    uint64_t value;
    __asm__ __volatile__("mrs %0, pmevcntr3_el0" : "=r"(value));
    return value;
}

static inline uint64_t l1_tlb_refill_counter(void) {
    uint64_t value;
    __asm__ __volatile__("mrs %0, pmevcntr4_el0" : "=r"(value));
    return value;
}
#else
#    error "unsupported arch"
#endif

struct iter {
    uint64_t cycles;
    uint64_t l1d_cache_access;
    uint64_t l2d_cache_access;
    uint64_t l1_tlb_refill;
    uint64_t mem_access;
    uint64_t num_exceptions;
};

#define NUM_ITERS 1024
static struct iter iters[NUM_ITERS];

static void print_stats(const char *name) {
    {
        uint64_t avg = 0, min = UINT64_MAX, max = 0;
        for (size_t i = 0; i < NUM_ITERS; i++) {
            min = MIN(min, iters[i].cycles);
            max = MAX(max, iters[i].cycles);
            avg += iters[i].cycles;
        }

        avg /= NUM_ITERS;
        METRIC(name, min);
        INFO("%s: cycles: avg=%d, min=%d, max=%d", name, avg, min, max);
    }

    {
        uint64_t avg = 0, min = UINT64_MAX, max = 0;
        for (size_t i = 0; i < NUM_ITERS; i++) {
            min = MIN(min, iters[i].l1d_cache_access);
            max = MAX(max, iters[i].l1d_cache_access);
            avg += iters[i].l1d_cache_access;
        }

        avg /= NUM_ITERS;
        INFO("%s: l1d_cache_access: avg=%d, min=%d, max=%d", name, avg, min,
             max);
    }

    {
        uint64_t avg = 0, min = UINT64_MAX, max = 0;
        for (size_t i = 0; i < NUM_ITERS; i++) {
            min = MIN(min, iters[i].l2d_cache_access);
            max = MAX(max, iters[i].l2d_cache_access);
            avg += iters[i].l2d_cache_access;
        }

        avg /= NUM_ITERS;
        INFO("%s: l2d_cache_access: avg=%d, min=%d, max=%d", name, avg, min,
             max);
    }

    {
        uint64_t avg = 0, min = UINT64_MAX, max = 0;
        for (size_t i = 0; i < NUM_ITERS; i++) {
            min = MIN(min, iters[i].mem_access);
            max = MAX(max, iters[i].mem_access);
            avg += iters[i].mem_access;
        }

        avg /= NUM_ITERS;
        INFO("%s: mem_access: avg=%d, min=%d, max=%d", name, avg, min, max);
    }

    {
        uint64_t avg = 0, min = UINT64_MAX, max = 0;
        for (size_t i = 0; i < NUM_ITERS; i++) {
            min = MIN(min, iters[i].l1_tlb_refill);
            max = MAX(max, iters[i].l1_tlb_refill);
            avg += iters[i].l1_tlb_refill;
        }

        avg /= NUM_ITERS;
        INFO("%s: l1_tlb_refill: avg=%d, min=%d, max=%d", name, avg, min, max);
    }

    {
        uint64_t avg = 0, min = UINT64_MAX, max = 0;
        for (size_t i = 0; i < NUM_ITERS; i++) {
            min = MIN(min, iters[i].num_exceptions);
            max = MAX(max, iters[i].num_exceptions);
            avg += iters[i].num_exceptions;
        }

        avg /= NUM_ITERS;
        INFO("%s: num_exceptions: avg=%d, min=%d, max=%d", name, avg, min, max);
    }
}

inline static void begin(int i) {
    iters[i].cycles = cycle_counter();
    iters[i].l1d_cache_access = l1d_cache_counter();
    iters[i].l2d_cache_access = l2d_cache_counter();
    iters[i].l1_tlb_refill = l1_tlb_refill_counter();
    iters[i].mem_access = mem_access_counter();
    iters[i].num_exceptions = exception_counter();
}

inline static void end(int i) {
    iters[i].cycles = cycle_counter() - iters[i].cycles;
    iters[i].l1d_cache_access = l1d_cache_counter() - iters[i].l1d_cache_access;
    iters[i].l2d_cache_access = l2d_cache_counter() - iters[i].l2d_cache_access;
    iters[i].l1_tlb_refill = l1_tlb_refill_counter() - iters[i].l1_tlb_refill;
    iters[i].mem_access = mem_access_counter() - iters[i].mem_access;
    iters[i].num_exceptions = exception_counter() - iters[i].num_exceptions;
}

void main(void) {
    INFO("starting IPC benchmark...");
    task_t server_task = ipc_lookup("benchmark_server");

    for (int i = 0; i < NUM_ITERS; i++) {
        begin(i);
        end(i);
    }
    print_stats("reading cycle counter");

    uint8_t *tmp1 = malloc(512);
    uint8_t *tmp2 = malloc(512);
    for (int i = 0; i < NUM_ITERS; i++) {
        begin(i);
        memcpy(tmp1, tmp2, 8);
        end(i);
    }
    print_stats("memcpy (8-bytes)");

    for (int i = 0; i < NUM_ITERS; i++) {
        begin(i);
        memcpy(tmp1, tmp2, 64);
        end(i);
    }
    print_stats("memcpy (64-bytes)");

    for (int i = 0; i < NUM_ITERS; i++) {
        begin(i);
        memcpy(tmp1, tmp2, 512);
        end(i);
    }
    print_stats("memcpy (512-bytes)");

    for (int i = 0; i < NUM_ITERS; i++) {
        begin(i);
        syscall(SYS_NOP, 0, 0, 0, 0, 0);
        end(i);
    }
    print_stats("nop syscall");

    //
    //  IPC round-trip benchmark
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        struct message m = {.type = BENCHMARK_NOP_MSG};
        begin(i);
        ipc_call(server_task, &m);
        end(i);
    }
    print_stats("IPC round-trip (simple)");

    //
    //  IPC round-trip benchmark (with small ool payload)
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        // Since we don't access the data (ool_payload) to be sent, the kernel
        // internally handles the page fault on the first message passing. Thus
        // it should take signficantly long on the first time.
        static char ool_payload[1] = "A";

        struct message m;
        m.type = BENCHMARK_NOP_WITH_OOL_MSG;
        m.benchmark_nop_with_ool.data = ool_payload;
        m.benchmark_nop_with_ool.data_len = PAGE_SIZE;

        begin(i);
        ipc_call(server_task, &m);
        end(i);
        ASSERT(m.type == BENCHMARK_NOP_WITH_OOL_REPLY_MSG);
        free(m.benchmark_nop_with_ool_reply.data);
    }
    print_stats("IPC round-trip (with 1-byte ool)");

    //
    //  IPC round-trip benchmark (with ool payload)
    //
    for (int i = 0; i < NUM_ITERS; i++) {
        // Since we don't access the data (ool_payload) to be sent, the kernel
        // internally handles the page fault on the first message passing. Thus
        // it should take signficantly long on the first time.
        static char ool_payload[PAGE_SIZE] = "This is a ool payload!";

        struct message m;
        m.type = BENCHMARK_NOP_WITH_OOL_MSG;
        m.benchmark_nop_with_ool.data = ool_payload;
        m.benchmark_nop_with_ool.data_len = PAGE_SIZE;

        begin(i);
        ipc_call(server_task, &m);
        end(i);
        ASSERT(m.type == BENCHMARK_NOP_WITH_OOL_REPLY_MSG);
        free(m.benchmark_nop_with_ool_reply.data);
    }
    print_stats("IPC round-trip (with PAGE_SIZE-sized ool)");
}
