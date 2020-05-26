#include <types.h>
#include <main.h>
#include <printk.h>
#include "peripherals.h"

void arch_idle(void) {
    // Turn the idle task into handler mode...
    register long r0 __asm__("r0") = SYS_NOP;
    __asm__ __volatile__("svc 0" : "=r"(r0) : "r"(r0) : "ip", "r6", "memory");

    // Returned back from the svc and now the idle (current) task is being
    // executed in handler mode.
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
