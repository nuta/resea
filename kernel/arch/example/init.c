#include <types.h>
#include <boot.h>
#include <task.h>
#include <printk.h>
#include <string.h>

extern char __bss[];
extern char __bss_end[];

void example_init(void) {
    memset(__bss, 0, (vaddr_t) __bss_end - (vaddr_t) __bss);
    lock();
}

void arch_idle(void) {
    for (;;);
}

void arch_semihosting_halt(void) {
}
