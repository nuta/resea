#include <endian.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/irq.h>
#include <string.h>
#include <driver/io.h>
#include <virtio/virtio.h>

/// The maximum number of virtqueues.
#define NUM_VIRTQS_MAX 8

static task_t dm_server;
static struct virtio_virtq virtqs[NUM_VIRTQS_MAX];
static io_t bar0_io = NULL;

static uint8_t read_device_status(void) {
    return io_read8(bar0_io, VIRTIO_REG_DEVICE_STATUS);
}

static void write_device_status(uint8_t value) {
    io_write8(bar0_io, VIRTIO_REG_DEVICE_STATUS, value);
}

static uint64_t read_device_features(void) {
    return io_read32(bar0_io, VIRTIO_REG_DEVICE_FEATS);
}

/// Reads the ISR status and de-assert an interrupt
/// ("4.1.4.5 ISR status capability").
static uint8_t read_isr_status(void) {
    return io_read8(bar0_io, VIRTIO_REG_ISR_STATUS);
}

/// Returns the number of descriptors in total in the queue.
static uint16_t virtq_num_descs(void) {
    return io_read16(bar0_io, VIRTIO_REG_NUM_DESCS);
}

/// Returns the `index`-th virtqueue.
static struct virtio_virtq *virtq_get(unsigned index) {
    return &virtqs[index];
}

/// Notifies the device that the queue contains a descriptor it needs to process.
static void virtq_notify(struct virtio_virtq *vq) {
    mb();
    io_write16(bar0_io, VIRTIO_REG_QUEUE_NOTIFY, vq->index);
}

/// Selects the current virtqueue in the common config.
static void virtq_select(unsigned index) {
    io_write16(bar0_io, VIRTIO_REG_QUEUE_SELECT, index);
}

/// Initializes a virtqueue.
static void virtq_init(unsigned index) {
    virtq_select(index);

    size_t num_descs = virtq_num_descs();
    ASSERT(num_descs <= 4096 && "too large queue size");

    offset_t avail_ring_off = sizeof(struct virtq_desc) * num_descs;
    size_t avail_ring_size = sizeof(uint16_t) * (3 + num_descs);
    offset_t used_ring_off = ALIGN_UP(avail_ring_off + avail_ring_size, PAGE_SIZE);
    size_t used_ring_size =
        sizeof(uint16_t) * 3 + sizeof(struct virtq_used_elem) * num_descs;
    size_t virtq_size = used_ring_off + ALIGN_UP(used_ring_size, PAGE_SIZE);

    dma_t virtq_dma =
        dma_alloc(virtq_size, DMA_ALLOC_TO_DEVICE | DMA_ALLOC_FROM_DEVICE);
    memset(dma_buf(virtq_dma), 0, virtq_size);

    vaddr_t base = (vaddr_t) dma_buf(virtq_dma);
    virtqs[index].index = index;
    virtqs[index].num_descs = num_descs;
    virtqs[index].legacy.virtq_dma = virtq_dma;
    virtqs[index].legacy.next_avail_index = 0;
    virtqs[index].legacy.last_used_index = 0;
    virtqs[index].legacy.descs = (struct virtq_desc *) base;
    virtqs[index].legacy.avail = (struct virtq_avail *) (base + avail_ring_off);
    virtqs[index].legacy.used = (struct virtq_used *) (base + used_ring_off);

    paddr_t paddr = dma_daddr(virtq_dma);
    ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));
    io_write32(bar0_io, VIRTIO_REG_QUEUE_ADDR_PFN, paddr / PAGE_SIZE);
}

static void activate(void) {
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER_OK);
}

/// Allocates a descriptor for the ouput to the device (e.g. TX virtqueue in
/// virtio-net). If `VIRTQ_DESC_F_NEXT` is set in `flags`, it allocates the next
/// descriptor (i.e. constructs a descriptor chain). If `prev_desc` is set, it
/// reuses the previously allocated descriptor.
static int virtq_alloc(struct virtio_virtq *vq, size_t len, uint16_t flags,
                       int prev_desc) {
    int desc_index = vq->legacy.next_avail_index % vq->num_descs;
    struct virtq_desc *desc = &vq->legacy.descs[desc_index];

    // TODO: FIXME: Check if the descriptor is available.

    // TODO: Use `descs[prev->desc]->next` if prev_desc != VIRTQ_ALLOC_NO_PREV.
    ASSERT(prev_desc == VIRTQ_ALLOC_NO_PREV && "not yet implemented");

    desc->len = into_le32(len);
    desc->flags = flags;
    desc->next = 0; // TODO:

    vq->legacy.next_avail_index++;
    return desc_index;
}

/// Returns the next descriptor which is already used by the device. If the
/// buffer is input from the device, call `virtq_push_desc` once you've handled
/// the input.
static error_t virtq_pop_desc(struct virtio_virtq *vq, int *index, size_t *len) {
    if (vq->legacy.last_used_index == vq->legacy.used->index) {
        return ERR_EMPTY;
    }

    struct virtq_used_elem *used_elem = &vq->legacy.used->ring[vq->legacy.last_used_index];
    *index = used_elem->id;
    *len = used_elem->len;
    vq->legacy.last_used_index++;
    return OK;
}

/// Adds the given head index of a decriptor chain to the avail ring and asks
/// the device to process it.
static void virtq_kick_desc(struct virtio_virtq *vq, int index) {
    vq->legacy.avail->ring[vq->legacy.avail->index % vq->num_descs] = index;
    mb();
    vq->legacy.avail->index++;
    virtq_notify(vq);
}

/// Makes the descriptor available for input from the device.
static void virtq_push_desc(struct virtio_virtq *vq, int index) {
    virtq_kick_desc(vq, index);
}

/// Allocates queue buffers. If `writable` is true, the buffers are initialized
/// as input ones from the device (e.g. RX virqueue in virtio-net).
static void virtq_allocate_buffers(struct virtio_virtq *vq, size_t buffer_size,
                                   bool writable) {
    dma_t dma = dma_alloc(buffer_size * vq->num_descs, DMA_ALLOC_FROM_DEVICE);
    vq->buffers_dma = dma;
    vq->buffers = dma_buf(dma);
    vq->buffer_size = buffer_size;

    uint16_t flags = writable ? VIRTQ_DESC_F_WRITE : 0;
    for (int i = 0; i < vq->num_descs; i++) {
        vq->legacy.descs[i].addr = into_le64(dma_daddr(dma) + (buffer_size * i));
        vq->legacy.descs[i].len = into_le32(buffer_size);
        vq->legacy.descs[i].flags = flags;
        vq->legacy.descs[i].next = 0;

        if (writable) {
            vq->legacy.avail->ring[i] = i;
            vq->legacy.avail->index++;
        }
    }
}

/// Checks and enables features. It aborts if any of the features is not supported.
static void negotiate_feature(uint64_t features) {
    // Abort if the device does not support features we need.
    ASSERT((read_device_features() & features) == features);
    io_write32(bar0_io, VIRTIO_REG_DRIVER_FEATS, features);
    write_device_status(read_device_status() | VIRTIO_STATUS_FEAT_OK);
    ASSERT((read_device_status() & VIRTIO_STATUS_FEAT_OK) != 0);
}

static uint32_t pci_config_read(handle_t device, unsigned offset, unsigned size) {
    struct message m;
    m.type = DM_PCI_READ_CONFIG_MSG;
    m.dm_pci_read_config.handle = device;
    m.dm_pci_read_config.offset = offset;
    m.dm_pci_read_config.size = size;
    ASSERT_OK(ipc_call(dm_server, &m));
    return m.dm_pci_read_config_reply.value;
}

static uint64_t read_device_config(offset_t offset, size_t size) {
    return io_read8(bar0_io, VIRTIO_REG_DEVICE_CONFIG_BASE + offset);
}

struct virtio_ops virtio_legacy_ops = {
    .read_device_features = read_device_features,
    .negotiate_feature = negotiate_feature,
    .read_device_config = read_device_config,
    .activate = activate,
    .read_isr_status = read_isr_status,
    .virtq_init = virtq_init,
    .virtq_get = virtq_get,
    .virtq_allocate_buffers = virtq_allocate_buffers,
    .virtq_alloc = virtq_alloc,
    .virtq_pop_desc = virtq_pop_desc,
    .virtq_push_desc = virtq_push_desc,
    .virtq_kick_desc = virtq_kick_desc,
    .virtq_notify = virtq_notify,
};

/// Looks for and initializes a virtio device with the given device type. It
/// sets the IRQ vector to `irq` on success.
error_t virtio_legacy_find_device(int device_type, struct virtio_ops **ops, uint8_t *irq) {
    // Search the PCI bus for a virtio device...
    dm_server = ipc_lookup("dm");
    struct message m;
    m.type = DM_ATTACH_PCI_DEVICE_MSG;
    m.dm_attach_pci_device.vendor_id = 0x1af4;
    m.dm_attach_pci_device.device_id = 0x1000;
    ASSERT_OK(ipc_call(dm_server, &m));
    handle_t pci_device = m.dm_attach_pci_device_reply.handle;

    uint32_t bar0 = pci_config_read(pci_device, 0x10, sizeof(uint32_t));
    ASSERT((bar0 & 1) == 1 && "BAR#0 should be io-mapped");

    bar0_io = io_alloc_port(bar0 & ~0b11, 32, IO_ALLOC_NORMAL);

    // Read the IRQ vector.
    *irq = pci_config_read(pci_device, 0x3c, sizeof(uint8_t));
    *ops = &virtio_legacy_ops;

    // Enable PCI bus master.
    m.type = DM_PCI_ENABLE_BUS_MASTER_MSG;
    m.dm_pci_enable_bus_master.handle = pci_device;
    ASSERT_OK(ipc_call(dm_server, &m));

    // "3.1.1 Driver Requirements: Device Initialization"
    write_device_status(0); // Reset the device.
    write_device_status(read_device_status() | VIRTIO_STATUS_ACK);
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER);

    TRACE("found a virtio-legacy device");
    return OK;
}
