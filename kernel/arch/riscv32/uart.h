#pragma once
#include "uart.h"

#define UART_ADDR ((paddr_t) 0x10000000)
#define UART0_IRQ 10

#define UART_THR (UART_ADDR + 0x00)
#define UART_RHR (UART_ADDR + 0x00)

/// Interrupt Enable Register.
#define UART_IER    (UART_ADDR + 0x01)
#define UART_IER_RX (1 << 0)

/// FIFO Control Register.
#define UART_FCR (UART_ADDR + 0x02)

/// Line Status Register.
#define UART_LSR          (UART_ADDR + 0x05)
#define UART_LSR_RX_READY (1 << 0)

#define UART_FCR_FIFO_ENABLE (1 << 0)
#define UART_FCR_FIFO_CLEAR  (0b11 << 1)

void riscv32_uart_init(void);
