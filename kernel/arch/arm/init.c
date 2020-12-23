#include <types.h>
#include <boot.h>
#include <printk.h>
#include <task.h>
#include <string.h>
#include <bootinfo.h>
#include <machine/peripherals.h>

void arch_idle(void) {
    while (true) {
        __asm__ __volatile__("wfi");
    }
}

static struct bootinfo bootinfo;
extern char __bss[];
extern char __bss_end[];

void arm_init(void) {
    memset(__bss, 0, (vaddr_t) __bss_end - (vaddr_t) __bss);
    lock();
    arm_peripherals_init();

    kmain(&bootinfo);

    PANIC("kmain returned");
    for (;;) {
        halt();
    }
}

void arch_semihosting_halt(void) {
    // ARM Semihosting
    uint32_t params[] = {
        0x20026, // application exit
        0,       // exit code
    };

    register uint32_t r0 __asm__("r0") = 0x20;    // exit
    register uint32_t r1 __asm__("r1") = (uint32_t) params;
    __asm__ __volatile__("bkpt 0xab" :: "r"(r0), "r"(r1));
}
