#include <types.h>
#include <main.h>
#include <printk.h>
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

void arm64_init(void) {
    // TODO: disable unused exception table vectors
    // TODO: smp
    // TODO: kdebug
    // TODO: improve tlb flushing

    ARM64_MSR(vbar_el1, &exception_vector);

    // We no longer need to access the lower addresses.
    ARM64_MSR(ttbr0_el1, 0ull);

    arm64_peripherals_init();
    kmain();

    PANIC("kmain returned");
    for (;;) {
        halt();
    }
}
