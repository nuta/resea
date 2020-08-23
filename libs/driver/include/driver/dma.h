#ifndef __DRIVER_DMA_H__
#define __DRIVER_DMA_H__

#include <types.h>

typedef paddr_t daddr_t;

struct dma {
    vaddr_t vaddr;
    daddr_t daddr;
};

typedef struct dma *dma_t;

#define DMA_ALLOC_TO_DEVICE   (1 << 0)
#define DMA_ALLOC_FROM_DEVICE (1 << 1)

dma_t dma_alloc(size_t len, unsigned flags);
void dma_free(dma_t dma);
void dma_flush_write(dma_t dma);
void dma_flush_read(dma_t dma);
daddr_t dma_daddr(dma_t dma);
vaddr_t dma_vaddr(dma_t dma);
uint8_t *dma_buf(dma_t dma);

#endif
