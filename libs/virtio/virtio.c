#include <endian.h>
#include <memory_acceess.h>
#include <resea/syscall.h>
#include <virtio/virtio.h>

/// The maximum number of virtqueues.
#define NUM_VIRTQS_MAX 8

static struct virtio_virtq virtqs[NUM_VIRTQS_MAX];

static uint8_t read_device_status(void) {
    return volatile_read8(base + VIRTIO_REG_DEVICE_STATUS);
}

static void write_device_status(uint8_t value) {
    volatile_write8(base + VIRTIO_REG_DEVICE_STATUS, value);
}

static uint64_t read_device_features(void) {
    return volatile_read32(base + VIRTIO_REG_DEVICE_FEATS);
}

/// Reads the ISR status and de-assert an interrupt
/// ("4.1.4.5 ISR status capability").
static uint8_t read_isr_status(void) {
    return volatile_read8(base + VIRTIO_REG_ISR_STATUS);
}

/// Returns the number of descriptors in total in the queue.
static uint16_t virtq_num_descs(void) {
    return volatile_read16(base + VIRTIO_REG_NUM_DESCS);
}

/// Returns the `index`-th virtqueue.
static struct virtio_virtq *virtq_get(unsigned index) {
    return &virtqs[index];
}

/// Notifies the device that the queue contains a descriptor it needs to
/// process.
static void virtq_notify(struct virtio_virtq *vq) {
    MEMORY_BARRIER();
    volatile_write16(base + VIRTIO_REG_QUEUE_NOTIFY, vq->index);
}

/// Selects the current virtqueue in the common config.
static void virtq_select(unsigned index) {
    volatile_write16(base + VIRTIO_REG_QUEUE_SELECT, index);
}

/// Initializes a virtqueue.
static void virtq_init(unsigned index) {
    virtq_select(index);

    size_t num_descs = virtq_num_descs();
    ASSERT(num_descs <= 4096 && "too large queue size");

    uint32_t avail_ring_off = sizeof(struct virtq_desc) * num_descs;
    size_t avail_ring_size = sizeof(uint16_t) * (3 + num_descs);
    uint32_t used_ring_off =
        ALIGN_UP(avail_ring_off + avail_ring_size, PAGE_SIZE);
    size_t used_ring_size =
        sizeof(uint16_t) * 3 + sizeof(struct virtq_used_elem) * num_descs;
    size_t virtq_size = used_ring_off + ALIGN_UP(used_ring_size, PAGE_SIZE);

    dma_t virtq_dma =
        dma_alloc(virtq_size, DMA_ALLOC_TO_DEVICE | DMA_ALLOC_FROM_DEVICE);
    memset(dma_buf(virtq_dma), 0, virtq_size);

    // TODO: error check
    paddr_t paddr = sys_pm_alloc(virtq_size, 0 /* FIXME: */);
    uaddr_t uaddr = sys_vm_map(paddr, virtq_size, 0 /* FIXME: */);

    struct virtio_virtq *vq = &virtqs[index];
    vq->index = index;
    vq->num_descs = num_descs;
    vq->legacy.virtq_dma = virtq_dma;
    vq->legacy.next_avail_index = 0;
    vq->legacy.last_used_index = 0;
    vq->legacy.descs = (struct virtq_desc *) uaddr;
    vq->legacy.avail = (struct virtq_avail *) (uaddr + avail_ring_off);
    vq->legacy.used = (struct virtq_used *) (uaddr + used_ring_off);

    // Add descriptors into the free list.
    vq->legacy.free_head = 0;
    vq->legacy.num_free_descs = num_descs;
    for (size_t i = 0; i < num_descs; i++) {
        vq->legacy.descs[i].next = (i + 1 == num_descs) ? 0 : i + 1;
    }

    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));
    io_write32(bar0_io, VIRTIO_REG_QUEUE_ADDR_PFN, paddr / PAGE_SIZE);
}
