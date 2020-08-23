#ifndef __DRIVER_IRQ_H__
#define __DRIVER_IRQ_H__

#include <types.h>

error_t irq_acquire(unsigned irq);
error_t irq_release(unsigned irq);

#endif
