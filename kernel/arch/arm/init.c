#include <types.h>
#include <main.h>
#include <printk.h>
#include <string.h>
#include "peripherals.h"

void arch_idle(void) {
    while (true) {
        __asm__ __volatile__("wfi");
    }
}

extern char __bss[];
extern char __bss_end[];

void arm_init(void) {
    memset(__bss, 0, (vaddr_t) __bss_end - (vaddr_t) __bss);
    arm_peripherals_init();
    kmain();

    PANIC("kmain returned");
    for (;;) {
        halt();
    }
}
