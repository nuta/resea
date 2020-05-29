#ifndef __RESEA_IO_H__
#define __RESEA_IO_H__

#include <arch/io.h>
#include <types.h>

error_t irq_acquire(unsigned irq);
error_t irq_release(unsigned irq);
void *io_alloc_pages(size_t num_pages, paddr_t map_to, paddr_t *paddr);

#endif
