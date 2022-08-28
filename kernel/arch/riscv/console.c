#include <kernel/arch.h>

#define UART_TX 0x10000000

void arch_console_write(const char *str) {
    for (; *str != '\0'; str++) {
        *((volatile char *) UART_TX) = *str;
    }
}
