#ifndef __SERVER_H__
#define __SERVER_H__

#include <ipc.h>

extern struct channel *kernel_server_ch;
void deliver_interrupt(uint8_t irq);
void kernel_server_init(void);

#endif
