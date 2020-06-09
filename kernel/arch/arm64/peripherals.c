#include <types.h>
#include <printk.h>
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

void arm_peripherals_init(void) {
}
