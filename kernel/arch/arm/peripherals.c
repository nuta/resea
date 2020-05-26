#include <types.h>
#include <printk.h>
#include "peripherals.h"

static inline uint32_t mmio_read(vaddr_t addr) {
    return *((volatile uint32_t *) addr);
}

static inline void mmio_write(vaddr_t addr, uint32_t value) {
    *((volatile uint32_t *) addr) = value;
}

void arch_printchar(char ch) {
    mmio_write(UART_TXD, ch);
    while(!mmio_read(UART_TXREADY));
    mmio_write(UART_TXREADY,0);
}

char kdebug_readchar(void) {
    return mmio_read(UART_RXD);
}

void arm_peripherals_init(void) {
    // UART: Set baudrate and RX/TX pins, enable interrupts.
    mmio_write(GPIO_DIRSET, (1 << UART_PIN_TX) | (0 << UART_PIN_RX));
    mmio_write(UART_BAUDRATE, UART_BAUDRATE_115200);
    mmio_write(UART_PSELTXD, UART_PIN_TX);
    mmio_write(UART_PSELRXD, UART_PIN_RX);
    mmio_write(UART_INTENSET, UART_INTENSET_RXDRDY);
    mmio_write(UART_ENABLE, 4);
    mmio_write(UART_STARTTX, 1);

    // SysTick Timer.
    uint32_t tenms = mmio_read(SYSTICK_CALIB) & 0xffffff;
    uint32_t onems = tenms / 10;
    TRACE("ticks per 1ms = %d", onems);
    ASSERT(onems > 0);
    mmio_write(SYSTICK_CVR, 0);
    mmio_write(SYSTICK_RVR, onems);
    mmio_write(SYSTICK_CSR, SYSTICK_CSR_TICKINT | SYSTICK_CSR_ENABLE);
}
