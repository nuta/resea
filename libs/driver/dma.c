#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/dma.h>

/// Allocates a DMA area which is accessible from the DMA controller.
dma_t dma_alloc(size_t len, unsigned flags) {
    struct message m;
    m.type = ALLOC_PAGES_MSG;
    m.alloc_pages.paddr = 0;
    m.alloc_pages.num_pages = ALIGN_UP(len, PAGE_SIZE) / PAGE_SIZE;
    error_t err = ipc_call(INIT_TASK, &m);
    ASSERT_OK(err);
    ASSERT(m.type == ALLOC_PAGES_REPLY_MSG);

    struct dma *dma = malloc(sizeof(*dma));
    dma->vaddr = m.alloc_pages_reply.vaddr;

    // Use paddr for the device address for now.
    dma->daddr = m.alloc_pages_reply.paddr;
    return dma;
}

/// Frees a DMA area.
void dma_free(dma_t dma) {
    // TODO:
}

/// Performs arch-specific pre-DMA work after writing into the DMA area.
void dma_flush_write(dma_t dma) {
    // Add arch-specific task with #ifdef if you need.

    // Prevent reordering the memory access.
    __sync_synchronize();
}

/// Performs arch-specific post-DMA work before reading from the DMA area.
void dma_flush_read(dma_t dma) {
    // Add arch-specific task with #ifdef if you need.

    // Prevent reordering the memory access.
    __sync_synchronize();
}

/// Returns the "device" or "bus" address, which is accessible from the DMA controller.
daddr_t dma_daddr(dma_t dma) {
    return dma->daddr;
}

/// Returns the virtual memory address which is accessible from CPU.
vaddr_t dma_vaddr(dma_t dma) {
    return dma->vaddr;
}

/// Returns the pointer to the DMA memory area.
uint8_t *dma_buf(dma_t dma) {
    return (uint8_t *) dma_vaddr(dma);
}

