#include <driver/io.h>
#include <driver/irq.h>
#include <endian.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include <virtio/virtio.h>

/// The maximum number of virtqueues.
#define NUM_VIRTQS_MAX 8

static task_t dm_server;
static struct virtio_virtq virtqs[NUM_VIRTQS_MAX];
static offset_t notify_off_multiplier;
static io_t common_cfg_io = NULL;
static offset_t common_cfg_off;
static io_t device_cfg_io = NULL;
static offset_t device_cfg_off;
static io_t notify_struct_io = NULL;
static offset_t notify_cap_off;
static io_t isr_struct_io = NULL;
static offset_t isr_cap_off;

#define _COMMON_CFG_READ(size, field)                                          \
    io_read##size(                                                             \
        common_cfg_io,                                                         \
        common_cfg_off + offsetof(struct virtio_pci_common_cfg, field))
#define VIRTIO_COMMON_CFG_READ8(field)  _COMMON_CFG_READ(8, field)
#define VIRTIO_COMMON_CFG_READ16(field) from_le16(_COMMON_CFG_READ(16, field))
#define VIRTIO_COMMON_CFG_READ32(field) from_le32(_COMMON_CFG_READ(32, field))

#define _COMMON_CFG_WRITE(size, field, value)                                  \
    io_write##size(                                                            \
        common_cfg_io,                                                         \
        common_cfg_off + offsetof(struct virtio_pci_common_cfg, field), value)
#define VIRTIO_COMMON_CFG_WRITE8(field, value)                                 \
    _COMMON_CFG_WRITE(8, field, value)
#define VIRTIO_COMMON_CFG_WRITE16(field, value)                                \
    _COMMON_CFG_WRITE(16, field, into_le32(value))
#define VIRTIO_COMMON_CFG_WRITE32(field, value)                                \
    _COMMON_CFG_WRITE(32, field, into_le32(value))

#define _DEVICE_CFG_READ(size, struct_name, field)                             \
    io_read##size(device_cfg_io, device_cfg_off + offsetof(struct_name, field))
#define VIRTIO_DEVICE_CFG_READ8(struct_name, field)                            \
    _DEVICE_CFG_READ(8, struct_name, field)
#define VIRTIO_DEVICE_CFG_READ16(struct_name, field)                           \
    from_le16(_DEVICE_CFG_READ(16, struct_name, field))
#define VIRTIO_DEVICE_CFG_READ32(struct_name, field)                           \
    from_le32(_DEVICE_CFG_READ(32, struct_name, field))

static uint8_t read_device_status(void) {
    return VIRTIO_COMMON_CFG_READ8(device_status);
}

static void write_device_status(uint8_t value) {
    VIRTIO_COMMON_CFG_WRITE8(device_status, value);
}

static uint64_t read_device_features(void) {
    // Select and read feature bits 0 to 31.
    VIRTIO_COMMON_CFG_WRITE32(device_feature_select, 0);
    uint32_t feats_lo = VIRTIO_COMMON_CFG_READ32(device_feature);

    // Select and read feature bits 32 to 63.
    VIRTIO_COMMON_CFG_WRITE32(device_feature_select, 1);
    uint32_t feats_hi = VIRTIO_COMMON_CFG_READ32(device_feature);

    return (((uint64_t) feats_hi) << 32) | feats_lo;
}

static void set_desc_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_desc_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_desc_hi, paddr >> 32);
}

static void set_driver_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_driver_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_driver_hi, paddr >> 32);
}

static void set_device_paddr(uint64_t paddr) {
    VIRTIO_COMMON_CFG_WRITE32(queue_device_lo, paddr & 0xffffffff);
    VIRTIO_COMMON_CFG_WRITE32(queue_device_hi, paddr >> 32);
}

/// Reads the ISR status and de-assert an interrupt
/// ("4.1.4.5 ISR status capability").
static uint8_t read_isr_status(void) {
    return io_read8(isr_struct_io, isr_cap_off);
}

/// Returns the number of descriptors in total in the queue.
static uint16_t virtq_size(void) {
    return VIRTIO_COMMON_CFG_READ16(queue_size);
}

/// Returns the `index`-th virtqueue.
static struct virtio_virtq *virtq_get(unsigned index) {
    return &virtqs[index];
}

/// Notifies the device that the queue contains a descriptor it needs to
/// process.
static void virtq_notify(struct virtio_virtq *vq) {
    io_write16(notify_struct_io, vq->modern.queue_notify_off, vq->index);
}

/// Returns true if the descriptor is available for the output to the device.
/// XXX: should we count modern.used_wrap_counter and use is_desc_used()
/// instead?
static bool is_desc_free(struct virtio_virtq *vq,
                         struct virtq_packed_desc *desc) {
    uint16_t flags = from_le16(desc->flags);
    int avail = !!(flags & VIRTQ_DESC_F_AVAIL);
    int used = !!(flags & VIRTQ_DESC_F_USED);
    return avail == used;
}

/// Returns true if the descriptor has been used by the device.
static bool is_desc_used(struct virtio_virtq *vq,
                         struct virtq_packed_desc *desc) {
    uint16_t flags = from_le16(desc->flags);
    int avail = !!(flags & VIRTQ_DESC_F_AVAIL);
    int used = !!(flags & VIRTQ_DESC_F_USED);
    return avail == used && used == vq->modern.used_wrap_counter;
}

/// Selects the current virtqueue in the common config.
static void virtq_select(unsigned index) {
    VIRTIO_COMMON_CFG_WRITE8(queue_select, index);
}

/// Initializes a virtqueue.
static void virtq_init(unsigned index) {
    virtq_select(index);

    size_t num_descs = virtq_size();
    ASSERT(num_descs < 1024 && "too large queue size");

    offset_t queue_notify_off =
        notify_cap_off
        + VIRTIO_COMMON_CFG_READ16(queue_notify_off) * notify_off_multiplier;

    // Allocate the descriptor area.
    size_t descs_size = num_descs * sizeof(struct virtq_packed_desc);
    dma_t descs_dma =
        dma_alloc(descs_size, DMA_ALLOC_TO_DEVICE | DMA_ALLOC_FROM_DEVICE);
    memset(dma_buf(descs_dma), 0, descs_size);

    // Allocate the driver area.
    dma_t driver_dma =
        dma_alloc(sizeof(struct virtq_event_suppress), DMA_ALLOC_TO_DEVICE);
    memset(dma_buf(driver_dma), 0, sizeof(struct virtq_event_suppress));

    // Allocate the device area.
    dma_t device_dma =
        dma_alloc(sizeof(struct virtq_event_suppress), DMA_ALLOC_TO_DEVICE);
    memset(dma_buf(device_dma), 0, sizeof(struct virtq_event_suppress));

    // Register physical addresses.
    set_desc_paddr(dma_daddr(descs_dma));
    set_driver_paddr(dma_daddr(driver_dma));
    set_device_paddr(dma_daddr(device_dma));
    VIRTIO_COMMON_CFG_WRITE16(queue_enable, 1);

    int *next_indices = malloc(sizeof(int) * num_descs);
    for (size_t i = 0; i < num_descs; i++) {
        next_indices[i] = -1;
    }

    virtqs[index].index = index;
    virtqs[index].descs_dma = descs_dma;
    virtqs[index].num_descs = num_descs;
    virtqs[index].next_indices = next_indices;
    virtqs[index].modern.descs =
        (struct virtq_packed_desc *) dma_buf(descs_dma);
    virtqs[index].modern.queue_notify_off = queue_notify_off;
    virtqs[index].modern.next_avail = 0;
    virtqs[index].modern.next_used = 0;
    virtqs[index].modern.avail_wrap_counter = 1;
    virtqs[index].modern.used_wrap_counter = 1;
}

static void activate(void) {
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER_OK);
}

/// Allocates a descriptor for the ouput to the device (e.g. TX virtqueue in
/// virtio-net). If `prev_desc` is set, it checks if it's the previously
/// allocated decriptor to ensure that they forms a descriptor chain.
static int virtq_alloc(struct virtio_virtq *vq, size_t len, uint16_t flags,
                       int prev_desc) {
    int index = vq->modern.next_avail;
    struct virtq_packed_desc *desc = &vq->modern.descs[index];

    ASSERT((prev_desc != VIRTQ_ALLOC_NO_PREV
            || prev_desc != ((index == 0) ? vq->num_descs - 1 : index - 1))
           && "(prev_desc + 1) == (next_desc) does not hold");

    if (!is_desc_free(vq, desc)) {
        return -1;
    }

    vq->next_indices[index] = -1;
    if (prev_desc != VIRTQ_ALLOC_NO_PREV) {
        vq->next_indices[prev_desc] = index;
    }

    flags &= ~(1 << VIRTQ_DESC_F_AVAIL_SHIFT);
    flags &= ~(1 << VIRTQ_DESC_F_USED_SHIFT);
    flags |= (vq->modern.avail_wrap_counter << VIRTQ_DESC_F_AVAIL_SHIFT)
             | (!vq->modern.avail_wrap_counter << VIRTQ_DESC_F_USED_SHIFT);

    desc->flags = into_le16(flags);
    desc->len = into_le32(len);
    desc->id = into_le16(index);

    vq->modern.next_avail++;
    if (vq->modern.next_avail == vq->num_descs) {
        vq->modern.avail_wrap_counter ^= 1;
        vq->modern.next_avail = 0;
    }

    return index;
}

/// Returns the next descriptor which is already filled by the device. If the
/// buffer is input from the device, call `virtq_push_desc` once you've handled
/// the input.
static error_t virtq_pop_desc(struct virtio_virtq *vq, int *index,
                              size_t *len) {
    struct virtq_packed_desc *desc = &vq->modern.descs[vq->modern.next_used];
    if (!is_desc_used(vq, desc)) {
        return ERR_NOT_FOUND;
    }

    // Skip until the next chain.
    int num_in_chain = 1;
    int id = desc->id;
    while (vq->next_indices[id] != -1) {
        num_in_chain++;
        id = vq->next_indices[id];
    }

    vq->modern.next_used = vq->modern.next_used + num_in_chain;
    if (vq->modern.next_used >= vq->num_descs) {
        vq->modern.used_wrap_counter ^= 1;
        vq->modern.next_used = 0;
    }

    *index = from_le16(desc->id);
    *len = from_le32(desc->len);
    return OK;
}

/// Adds the given head index of a decriptor chain to the avail ring and asks
/// the device to process it.
static void virtq_kick_desc(struct virtio_virtq *vq, int index) {
    virtq_notify(vq);
}

/// Makes the descriptor available for input from the device.
static void virtq_push_desc(struct virtio_virtq *vq, int index) {
    struct virtq_packed_desc *desc = &vq->modern.descs[index];
    uint16_t flags =
        VIRTQ_DESC_F_WRITE
        | (!vq->modern.avail_wrap_counter << VIRTQ_DESC_F_AVAIL_SHIFT)
        | (vq->modern.avail_wrap_counter << VIRTQ_DESC_F_USED_SHIFT);

    desc->len = into_le32(vq->buffer_size);
    desc->flags = into_le16(flags);
}

/// Allocates queue buffers. If `writable` is true, the buffers are initialized
/// as input ones from the device (e.g. RX virqueue in virtio-net).
static void virtq_allocate_buffers(struct virtio_virtq *vq, size_t buffer_size,
                                   bool writable) {
    dma_t dma = dma_alloc(buffer_size * vq->num_descs, DMA_ALLOC_FROM_DEVICE);
    vq->buffers_dma = dma;
    vq->buffers = dma_buf(dma);
    vq->buffer_size = buffer_size;

    uint16_t flags = writable ? (VIRTQ_DESC_F_AVAIL | VIRTQ_DESC_F_WRITE) : 0;
    for (int i = 0; i < vq->num_descs; i++) {
        vq->modern.descs[i].id = into_le16(i);
        vq->modern.descs[i].addr =
            into_le64(dma_daddr(dma) + (buffer_size * i));
        vq->modern.descs[i].len = into_le32(buffer_size);
        vq->modern.descs[i].flags = into_le16(flags);
    }
}

/// Checks and enables features. It aborts if any of the features is not
/// supported.
static void negotiate_feature(uint64_t features) {
    features |= VIRTIO_F_VERSION_1 | VIRTIO_F_RING_PACKED;

    // Abort if the device does not support features we need.
    ASSERT((read_device_features() & features) == features);

    // Select and set feature bits 0 to 31.
    VIRTIO_COMMON_CFG_WRITE32(driver_feature_select, 0);
    VIRTIO_COMMON_CFG_WRITE32(driver_feature, features & 0xffffffff);

    // Select and set feature bits 32 to 63.
    VIRTIO_COMMON_CFG_WRITE32(driver_feature_select, 1);
    VIRTIO_COMMON_CFG_WRITE32(driver_feature, features >> 32);

    write_device_status(read_device_status() | VIRTIO_STATUS_FEAT_OK);
    ASSERT((read_device_status() & VIRTIO_STATUS_FEAT_OK) != 0);
}

static uint32_t pci_config_read(handle_t device, unsigned offset,
                                unsigned size) {
    struct message m;
    m.type = DM_PCI_READ_CONFIG_MSG;
    m.dm_pci_read_config.handle = device;
    m.dm_pci_read_config.offset = offset;
    m.dm_pci_read_config.size = size;
    ASSERT_OK(ipc_call(dm_server, &m));
    return m.dm_pci_read_config_reply.value;
}

static uint64_t read_device_config(offset_t offset, size_t size) {
    switch (size) {
        case sizeof(uint8_t):
            return io_read8(device_cfg_io, device_cfg_off + offset);
        case sizeof(uint16_t):
            return io_read16(device_cfg_io, device_cfg_off + offset);
        case sizeof(uint32_t):
            return io_read32(device_cfg_io, device_cfg_off + offset);
        default:
            UNREACHABLE();
    }
}

static int get_next_index(struct virtio_virtq *vq, int index) {
    ASSERT(index < vq->num_descs);
    return vq->next_indices[index];
}

static void *get_buffer(struct virtio_virtq *vq, int index) {
    return vq->buffers + index * vq->buffer_size;
}

struct virtio_ops virtio_modern_ops = {
    .read_device_features = read_device_features,
    .negotiate_feature = negotiate_feature,
    .read_device_config = read_device_config,
    .activate = activate,
    .read_isr_status = read_isr_status,
    .virtq_init = virtq_init,
    .virtq_get = virtq_get,
    .virtq_size = virtq_size,
    .virtq_allocate_buffers = virtq_allocate_buffers,
    .virtq_alloc = virtq_alloc,
    .virtq_pop_desc = virtq_pop_desc,
    .virtq_push_desc = virtq_push_desc,
    .virtq_kick_desc = virtq_kick_desc,
    .virtq_notify = virtq_notify,
    .get_next_index = get_next_index,
    .get_buffer = get_buffer,
};

static uint16_t get_transitional_device_id(int device_type) {
    switch (device_type) {
        case VIRTIO_DEVICE_NET: return 0x1000;
        default: return 0;
    }
}

/// Looks for and initializes a virtio device with the given device type. It
/// sets the IRQ vector to `irq` on success.
error_t virtio_modern_find_device(int device_type, struct virtio_ops **ops,
                                  uint8_t *irq) {
    // Search the PCI bus for a virtio device.
    //
    // "Devices MUST have the PCI Vendor ID 0x1AF4. Devices MUST either have the
    // PCI Device ID calculated by adding 0x1040 to the Virtio Device ID ... or
    // have the Transitional PCI Device ID"
    //
    // From "4.1.2.1 Device Requirements: PCI Device Discovery"
    dm_server = ipc_lookup("dm");
    uint16_t device_ids[] = {
        0x1040 + device_type,
        get_transitional_device_id(device_type),
        0,
    };

    handle_t pci_device = 0;
    for (int i = 0; device_ids[i] != 0; i++) {
        struct message m;
        m.type = DM_ATTACH_PCI_DEVICE_MSG;
        m.dm_attach_pci_device.vendor_id = 0x1af4;
        m.dm_attach_pci_device.device_id = device_ids[i];
        error_t err = ipc_call(dm_server, &m);
        switch (err) {
            case OK:
                break;
            case ERR_NOT_FOUND:
                continue;
            default: return err;
        }

        pci_device = m.dm_attach_pci_device_reply.handle;
        break;
    }

    if (!pci_device) {
        return ERR_NOT_FOUND;
    }

    // Walk capabilities list. A capability consists of the following fields
    // (from "4.1.4 Virtio Structure PCI Capabilities"):
    //
    // struct virtio_pci_cap {
    //     u8 cap_vndr;    /* Generic PCI field: PCI_CAP_ID_VNDR */
    //     u8 cap_next;    /* Generic PCI field: next ptr. */
    //     u8 cap_len;     /* Generic PCI field: capability length */
    //     u8 cfg_type;    /* Identifies the structure. */
    //     u8 bar;         /* Where to find it. */
    //     u8 padding[3];  /* Pad to full dword. */
    //     le32 offset;    /* Offset within bar. */
    //     le32 length;    /* Length of the structure, in bytes. */
    // };
    uint8_t cap_off = pci_config_read(pci_device, 0x34, sizeof(uint8_t));
    while (cap_off != 0) {
        uint8_t cap_id = pci_config_read(pci_device, cap_off, sizeof(uint8_t));
        uint8_t cfg_type =
            pci_config_read(pci_device, cap_off + 3, sizeof(uint8_t));
        uint8_t bar_index =
            pci_config_read(pci_device, cap_off + 4, sizeof(uint8_t));

        if (cap_id == 9 && cfg_type == VIRTIO_PCI_CAP_COMMON_CFG) {
            uint32_t bar = pci_config_read(pci_device, 0x10 + 4 * bar_index, 4);
            ASSERT((bar & 1) == 0
                   && "only supports memory-mapped I/O access for now");
            uint32_t bar_base = bar & ~0xf;
            size_t size = pci_config_read(pci_device, cap_off + 12, 4);
            common_cfg_off = pci_config_read(pci_device, cap_off + 8, 4);
            common_cfg_io = io_alloc_memory_fixed(
                bar_base, ALIGN_UP(common_cfg_off + size, PAGE_SIZE),
                IO_ALLOC_CONTINUOUS);
        }

        if (cap_id == 9 && cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG) {
            // Device-specific configuration space.
            uint32_t bar = pci_config_read(pci_device, 0x10 + 4 * bar_index, 4);
            ASSERT((bar & 1) == 0
                   && "only supports memory-mapped I/O access for now");
            uint32_t bar_base = bar & ~0xf;
            size_t size = pci_config_read(pci_device, cap_off + 12, 4);
            device_cfg_off = pci_config_read(pci_device, cap_off + 8, 4);
            device_cfg_io = io_alloc_memory_fixed(
                bar_base, ALIGN_UP(device_cfg_off + size, PAGE_SIZE),
                IO_ALLOC_CONTINUOUS);
        }

        if (cap_id == 9 && cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG) {
            // Notification structure:
            //
            // struct virtio_pci_notify_cap {
            //     struct virtio_pci_cap cap;
            //     le32 notify_off_multiplier; /* Multiplier for
            //     queue_notify_off. */
            // };
            notify_cap_off = pci_config_read(pci_device, cap_off + 8, 4);
            notify_off_multiplier =
                pci_config_read(pci_device, cap_off + 16, 4);
            uint32_t bar = pci_config_read(pci_device, 0x10 + 4 * bar_index, 4);
            ASSERT((bar & 1) == 0
                   && "only supports memory-mapped I/O access for now");
            uint32_t bar_base = bar & ~0xf;
            size_t size = pci_config_read(pci_device, cap_off + 12, 4);
            notify_struct_io = io_alloc_memory_fixed(
                bar_base, ALIGN_UP(notify_cap_off + size, PAGE_SIZE),
                IO_ALLOC_CONTINUOUS);
        }

        if (cap_id == 9 && cfg_type == VIRTIO_PCI_CAP_ISR_CFG) {
            // Notification structure:
            //
            // struct virtio_isr_cap {
            //     u8 isr_status;
            // };
            isr_cap_off = pci_config_read(pci_device, cap_off + 8, 4);
            uint32_t bar = pci_config_read(pci_device, 0x10 + 4 * bar_index, 4);
            ASSERT((bar & 1) == 0
                   && "only supports memory-mapped I/O access for now");
            uint32_t bar_base = bar & ~0xf;
            size_t size = pci_config_read(pci_device, cap_off + 12, 4);
            isr_struct_io = io_alloc_memory_fixed(
                bar_base, ALIGN_UP(isr_cap_off + size, PAGE_SIZE),
                IO_ALLOC_CONTINUOUS);
        }

        cap_off = pci_config_read(pci_device, cap_off + 1, sizeof(uint8_t));
    }

    if (!common_cfg_io || !device_cfg_io || !notify_struct_io
        || !isr_struct_io) {
        WARN_DBG("missing a BAR for the device access");
        return ERR_NOT_FOUND;
    }

    // Read the IRQ vector.
    *irq = pci_config_read(pci_device, 0x3c, sizeof(uint8_t));
    *ops = &virtio_modern_ops;

    // Enable PCI bus master.
    struct message m;
    m.type = DM_PCI_ENABLE_BUS_MASTER_MSG;
    m.dm_pci_enable_bus_master.handle = pci_device;
    ASSERT_OK(ipc_call(dm_server, &m));

    // "3.1.1 Driver Requirements: Device Initialization"
    write_device_status(0);  // Reset the device.
    write_device_status(read_device_status() | VIRTIO_STATUS_ACK);
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER);

    return OK;
}
