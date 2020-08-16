#include <types.h>
#include <printk.h>
#include "asm.h"
#include "peripherals.h"

void uart_send(unsigned int c) {
    while (mmio_read(UART0_FR) & (1 << 5)) {}
    mmio_write(UART0_DR, c);
}

char uart_getc() {
    while (mmio_read(UART0_FR) & (1 << 4)) {}
    return mmio_read(UART0_DR);
}

void arch_printchar(char ch) {
    uart_send(ch);
}

char kdebug_readchar(void) {
    return '\0'; // TODO:
}

bool kdebug_is_readable(void) {
    // TODO: Not yet implemented.
    return false;
}

void arm64_timer_reload(void) {
    uint64_t hz = ARM64_MRS(cntfrq_el0);
    ASSERT(hz >= 1000);
    hz /= 1000;
    ARM64_MSR(cntv_tval_el0, hz);
}

static void timer_init(void) {
    arm64_timer_reload();
    ARM64_MSR(cntv_ctl_el0, 1ull);
    mmio_write(CORE0_TIMER_IRQCNTL, 1 << 3 /* Enable nCNTVIRQ IRQ */);
}

void arm64_peripherals_init(void) {
    timer_init();
}

void arch_enable_irq(unsigned irq) {
    // TODO:
}

void arch_disable_irq(unsigned irq) {
    // TODO:
}
