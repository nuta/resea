#ifndef __SERVER_H__
#define __SERVER_H__

#include <ipc.h>

struct irq_listener {
    struct list_head next;
    uint8_t irq;
    struct channel *ch;
};

extern struct channel *kernel_server_ch;
void deliver_interrupt(uint8_t irq);
void kernel_server_init(void);

#endif
