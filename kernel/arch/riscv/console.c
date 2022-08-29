#include <kernel/arch.h>

#define UART_TX 0x10000000

void arch_console_write(char ch) {
    *((volatile char *) UART_TX) = ch;
}
