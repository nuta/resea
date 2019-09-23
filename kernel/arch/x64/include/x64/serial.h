#ifndef __X64_SERIAL_H__
#define __X64_SERIAL_H__

#define IOPORT_SERIAL 0x3f8
#define SERIAL_IRQ 4
#define RBR 0
#define DLL 0
#define DLH 1
#define IER 1
#define FCR 2
#define LCR 3
#define LSR 5
#define TX_READY 0x20

void x64_serial_putchar(char ch);
void x64_serial_early_init(void);
void x64_serial_init(void);

#endif
