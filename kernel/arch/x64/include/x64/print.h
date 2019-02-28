#ifndef __X64_PRINT_H__
#define __X64_PRINT_H__

#define IOPORT_SERIAL 0x3f8
#define IER 1
#define DLL 0
#define DLH 1
#define LCR 3
#define FCR 2
#define LSR 5
#define TX_READY 0x20

void x64_print_init(void);

#endif
