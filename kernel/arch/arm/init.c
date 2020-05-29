#include <types.h>
#include <main.h>
#include <printk.h>
#include "peripherals.h"

void arch_idle(void) {
    while (true) {
        __asm__ __volatile__("wfi");
    }
}

void arm_init(void) {
    arm_peripherals_init();
    kmain();

    PANIC("kmain returned");
    for (;;) {
        halt();
    }
}
