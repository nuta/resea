#ifndef __ARM_PERIPHERALS_H__
#define __ARM_PERIPHERALS_H__

#define GPIO_BASE   0x50000000
#define GPIO_DIRSET (GPIO_BASE + 0x518)

#define UART_BASE     0x40002000
#define UART_STARTTX  (UART_BASE + 0x008)
#define UART_TXREADY  (UART_BASE + 0x11c)
#define UART_INTENSET (UART_BASE + 0x304)
#define UART_ENABLE   (UART_BASE + 0x500)
#define UART_PSELTXD  (UART_BASE + 0x50c)
#define UART_PSELRXD  (UART_BASE + 0x514)
#define UART_RXD      (UART_BASE + 0x518)
#define UART_TXD      (UART_BASE + 0x51c)
#define UART_BAUDRATE (UART_BASE + 0x524)

#define UART_PIN_TX          24
#define UART_PIN_RX          25
#define UART_BAUDRATE_115200 0x01d7e000
#define UART_INTENSET_RXDRDY (1 << 2)

#define SYSTICK_CSR         0xe000e010
#define SYSTICK_CSR_ENABLE  (1 << 0)
#define SYSTICK_CSR_TICKINT (1 << 1)
#define SYSTICK_RVR         0xe000e014
#define SYSTICK_CVR         0xe000e018
#define SYSTICK_CALIB       0xe000e01c

void arm_peripherals_init(void);

#endif
