#include "plic.h"
#include "asm.h"
#include "uart.h"

int plic_pending(void) {
    int hart = CPUVAR->id;
    return mmio_read32(PLIC_SCLAIM(hart));
}

void plic_ack(int irq) {
    int hart = CPUVAR->id;
    mmio_write32(PLIC_SCLAIM(hart), irq);
}

void riscv32_plic_init(void) {
    mmio_write32(PLIC_ADDR + UART0_IRQ * 4, 1);

    int hart = CPUVAR->id;
    mmio_write32(PLIC_SENABLE(hart), 1 << UART0_IRQ);
    mmio_write32(PLIC_SPRIORITY(hart), 0);
}
