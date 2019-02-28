#include <x64/print.h>
#include <x64/asm.h>

void x64_print_init(void) {
    int baud = 9600;
    int divisor = 115200 / baud;
    asm_out8(IOPORT_SERIAL + IER, 0x00); // Disable interrupts
    asm_out8(IOPORT_SERIAL + DLL, divisor & 0xff);
    asm_out8(IOPORT_SERIAL + DLH, (divisor >> 8) & 0xff);
    asm_out8(IOPORT_SERIAL + LCR, 0x03); // 8n1
    asm_out8(IOPORT_SERIAL + FCR, 0x00); // No FIFO
}


void arch_putchar(char ch) {
    while ((asm_in8(IOPORT_SERIAL + LSR) & TX_READY) == 0);
    asm_out8(IOPORT_SERIAL, ch);
}
