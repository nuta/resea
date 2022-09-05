#include "uart.h"
#include "asm.h"
#include <kernel/arch.h>

void arch_console_write(char ch) {
    mmio_write8(UART_THR, ch);
}

int arch_console_read(void) {
    if ((mmio_read8(UART_LSR) & UART_LSR_RX_READY) == 0) {
        return -1;
    }

    return mmio_read8(UART_RHR);
}

void riscv32_uart_init(void) {
    // Disable interrupts.
    mmio_write8(UART_IER, 0);

    // Clear and enable FIFO.
    mmio_write8(UART_FCR, UART_FCR_FIFO_ENABLE | UART_FCR_FIFO_CLEAR);

    // Enable interrupts.
    mmio_write8(UART_IER, UART_IER_RX);
}
