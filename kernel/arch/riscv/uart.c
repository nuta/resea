#include "uart.h"
#include <kernel/arch.h>

void arch_console_write(char ch) {
    *((volatile char *) UART_TX) = ch;
}
