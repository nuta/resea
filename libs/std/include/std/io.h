#ifndef __IO_H__
#define __IO_H__

#include <arch/io.h>
#include <types.h>

void *io_alloc_pages(size_t num_pages, paddr_t map_to, paddr_t *paddr);

#endif
