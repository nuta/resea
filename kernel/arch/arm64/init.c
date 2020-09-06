#include <types.h>
#include <boot.h>
#include <task.h>
#include <printk.h>
#include <string.h>
#include "asm.h"
#include "peripherals.h"

extern uint8_t exception_vector;

void arch_idle(void) {
    task_switch();

    // Enable IRQ.
    __asm__ __volatile__("msr daifclr, #2");

    while (true) {
        __asm__ __volatile__("wfi");
    }
}

extern char __bss[];
extern char __bss_end[];

void arm64_init(void) {
    // TODO: disable unused exception table vectors
    // TODO: smp
    // TODO: kdebug
    // TODO: improve tlb flushing
    ARM64_MSR(vbar_el1, &exception_vector);

    // We no longer need to access the lower addresses.
    ARM64_MSR(ttbr0_el1, 0ull);

    bzero(get_cpuvar(), sizeof(struct cpuvar));
    bzero(__bss, (vaddr_t) __bss_end - (vaddr_t) __bss);
    lock();

    arm64_peripherals_init();
    kmain();

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
