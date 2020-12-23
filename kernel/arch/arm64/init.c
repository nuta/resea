#include <types.h>
#include <boot.h>
#include <task.h>
#include <printk.h>
#include <string.h>
#include <machine/peripherals.h>
#include <bootinfo.h>
#include "asm.h"

extern uint8_t exception_vector;

void arch_idle(void) {
    task_switch();

    while (true) {
        unlock();
        // Enable IRQ.
        __asm__ __volatile__("msr daifclr, #2");
        __asm__ __volatile__("wfi");
        // Disable IRQ.
        __asm__ __volatile__("msr daifset, #2");
        lock();
    }
}

extern char __bss[];
extern char __bss_end[];

__noreturn void mpinit(void) {
    while (true) {
        __asm__ __volatile__("msr daifset, #2");
        __asm__ __volatile__("wfe");
    }

    mpmain();

    PANIC("mpmain returned");
    for (;;) {
        halt();
    }
}

static struct bootinfo bootinfo;

void arm64_init(void) {
    // TODO: disable unused exception table vectors
    // TODO: smp
    // TODO: kdebug
    // TODO: improve tlb flushing
    ARM64_MSR(vbar_el1, &exception_vector);

    if (!mp_is_bsp()) {
        lock();
        mpinit();
        UNREACHABLE();
    }

    // We no longer need to access the lower addresses.
    ARM64_MSR(ttbr0_el1, 0ull);

    bzero(get_cpuvar(), sizeof(struct cpuvar));
    bzero(__bss, (vaddr_t) __bss_end - (vaddr_t) __bss);

    arm64_peripherals_init();
    lock();

    // Enable d-cache and i-cache.
    ARM64_MSR(sctlr_el1, ARM64_MRS(sctlr_el1) | (1 << 2) | (1 << 12));

    // Initialize the performance counter for benchmarking.
    ARM64_MSR(pmcr_el0, 0b1ull);           // Reset counters.
    ARM64_MSR(pmcr_el0, 0b11ull);          // Enable counters.

    // QEMU does not support performance counters available in the real Raspberry
    // Pi 3B+.
#ifndef CONFIG_SEMIHOSTING
    int num_perf_counters = (ARM64_MRS(pmcr_el0) >> 11) & 0b11111;
    ASSERT(num_perf_counters >= 5);

    uint32_t pmceid = ARM64_MRS(pmceid0_el0);
    ASSERT((pmceid & (1 << 4)) != 0 && "L1D_CACHE event is not supported");
    ASSERT((pmceid & (1 << 22)) != 0 && "L2D_CACHE event is not supported");
    ASSERT((pmceid & (1 << 19)) != 0 && "MEM_ACCESS event is not supported");
    ASSERT((pmceid & (1 << 9)) != 0 && "EXC_TAKEN event is not supported");
    ASSERT((pmceid & (1 << 5)) != 0 && "L1D_TLB_REFILL event is not supported");
    ARM64_MSR(pmevtyper0_el0, 0x04ull);
    ARM64_MSR(pmevtyper1_el0, 0x16ull);
    ARM64_MSR(pmevtyper2_el0, 0x13ull);
    ARM64_MSR(pmevtyper3_el0, 0x0aull);
    ARM64_MSR(pmevtyper4_el0, 0x05ull);
#endif

    ARM64_MSR(pmcntenset_el0, 0x8000001full); // Enable the cycle and 5 event counters.
    ARM64_MSR(pmuserenr_el0, 0b11ull);     // Enable user access to the counters.

    // FIXME: machine-specific
    bootinfo.memmap[0].base = (vaddr_t) __kernel_image_end;
    bootinfo.memmap[0].len = 128 * 1024 * 1024; // 128MiB
    bootinfo.memmap[0].type = BOOTINFO_MEMMAP_TYPE_AVAILABLE;
    kmain(&bootinfo);

    PANIC("kmain returned");
    for (;;) {
        halt();
    }
}

void arch_semihosting_halt(void) {
    // ARM Semihosting
    uint64_t params[] = {
        0x20026, // application exit
        0,       // exit code
    };

    register uint64_t x0 __asm__("x0") = 0x20; // exit
    register uint64_t x1 __asm__("x1") = (uint64_t) params;
    __asm__ __volatile__("hlt #0xf000" :: "r"(x0), "r"(x1));
}
