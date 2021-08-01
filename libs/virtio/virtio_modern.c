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
    size_t descs_size = num_descs * sizeof(struct virtq_desc);
    dma_t descs_dma =
        dma_alloc(descs_size, DMA_ALLOC_TO_DEVICE | DMA_ALLOC_FROM_DEVICE);
    memset(dma_buf(descs_dma), 0, descs_size);

    // Allocate the driver area.
    size_t avail_ring_size = num_descs * sizeof(struct virtq_avail);
    dma_t driver_dma = dma_alloc(avail_ring_size, DMA_ALLOC_TO_DEVICE);
    memset(dma_buf(driver_dma), 0, avail_ring_size);

    // Allocate the device area.
    size_t used_ring_size = num_descs * sizeof(struct virtq_used);
    dma_t device_dma = dma_alloc(used_ring_size, DMA_ALLOC_TO_DEVICE);
    memset(dma_buf(device_dma), 0, used_ring_size);

    // Register physical addresses.
    set_desc_paddr(dma_daddr(descs_dma));
    set_driver_paddr(dma_daddr(driver_dma));
    set_device_paddr(dma_daddr(device_dma));
    VIRTIO_COMMON_CFG_WRITE16(queue_enable, 1);

    int *next_indices = malloc(sizeof(int) * num_descs);
    for (size_t i = 0; i < num_descs; i++) {
        next_indices[i] = -1;
    }

    struct virtio_virtq *vq = &virtqs[index];
    vq->index = index;
    vq->num_descs = num_descs;
    vq->modern.queue_notify_off = queue_notify_off;
    vq->modern.next_avail_index = 0;
    vq->modern.last_used_index = 0;
    vq->modern.descs = (struct virtq_desc *) dma_buf(descs_dma);
    vq->modern.avail = (struct virtq_avail *) dma_buf(driver_dma);
    vq->modern.used = (struct virtq_used *) dma_buf(device_dma);

    // Add descriptors into the free list.
    vq->modern.free_head = 0;
    vq->modern.num_free_descs = num_descs;
    for (size_t i = 0; i < num_descs; i++) {
        vq->modern.descs[i].next = (i + 1 == num_descs) ? 0 : i + 1;
    }
}

static void activate(void) {
    write_device_status(read_device_status() | VIRTIO_STATUS_DRIVER_OK);
}

/// Enqueues a chain of descriptors into the virtq. Don't forget to call
/// `notify` to start processing the enqueued request.
static error_t virtq_push(struct virtio_virtq *vq,
                          struct virtio_chain_entry *chain, int n) {
    DEBUG_ASSERT(n > 0);
    if (n > vq->modern.num_free_descs) {
        // Try freeing used descriptors.
        while (vq->modern.last_used_index != vq->modern.used->index) {
            struct virtq_used_elem *used_elem =
                &vq->modern.used
                     ->ring[vq->modern.last_used_index % vq->num_descs];

            // Count the number of descriptors in the chain.
            int num_freed = 0;
            int next_desc_index = used_elem->id;
            while (true) {
                struct virtq_desc *desc = &vq->modern.descs[next_desc_index];
                num_freed++;

                if ((desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
                    break;
                }

                next_desc_index = desc->next;
            }

            // Enqueue the chain back into the free list.
            vq->modern.free_head = used_elem->id;
            vq->modern.num_free_descs += num_freed;
            vq->modern.last_used_index++;
        }
    }

    if (n > vq->modern.num_free_descs) {
        return ERR_NO_MEMORY;
    }

    int head_index = vq->modern.free_head;
    int desc_index = head_index;
    struct virtq_desc *desc = NULL;
    for (int i = 0; i < n; i++) {
        struct virtio_chain_entry *e = &chain[i];
        desc = &vq->modern.descs[desc_index];
        desc->addr = into_le64(e->addr);
        desc->len = into_le32(e->len);
        desc->flags =
            (e->device_writable ? VIRTQ_DESC_F_WRITE : 0) | VIRTQ_DESC_F_NEXT;
        desc_index = desc->next;
    }

    // Update the last entry in the chain.
    DEBUG_ASSERT(desc != NULL);
    int unused_next = desc->next;
    desc->next = 0;
    desc->flags &= ~VIRTQ_DESC_F_NEXT;

    vq->modern.free_head = unused_next;
    vq->modern.num_free_descs -= n;

    // Append the chain into the avail ring.
    vq->modern.avail->ring[vq->modern.avail->index % vq->num_descs] =
        head_index;
    mb();
    vq->modern.avail->index++;
    return OK;
}

/// Pops a descriptor chain processed by the device. Returns the number of
/// descriptors in the chain and fills `chain` with the popped descriptors.
///
/// If no chains in the used ring, it returns ERR_EMPTY.
static int virtq_pop(struct virtio_virtq *vq, struct virtio_chain_entry *chain,
                     int n, size_t *total_len) {
    if (vq->modern.last_used_index == vq->modern.used->index) {
        return ERR_EMPTY;
    }

    struct virtq_used_elem *used_elem =
        &vq->modern.used->ring[vq->modern.last_used_index % vq->num_descs];

    *total_len = used_elem->len;
    int next_desc_index = used_elem->id;
    struct virtq_desc *desc = NULL;
    int num_popped = 0;
    while (num_popped < n) {
        desc = &vq->modern.descs[next_desc_index];
        chain[num_popped].addr = desc->addr;
        chain[num_popped].len = desc->len;
        chain[num_popped].device_writable =
            (desc->flags & VIRTQ_DESC_F_WRITE) != 0;

        num_popped++;

        bool has_next = (desc->flags & VIRTQ_DESC_F_NEXT) != 0;
        if (!has_next) {
            break;
        }

        if (num_popped >= n && has_next) {
            // `n` is too short.
            return ERR_NO_MEMORY;
        }

        next_desc_index = desc->next;
    }

    // Prepend the popped descriptors into the free list.
    DEBUG_ASSERT(desc != NULL);
    desc->next = vq->modern.free_head;
    vq->modern.free_head = used_elem->id;
    vq->modern.num_free_descs += num_popped;

    vq->modern.last_used_index++;
    return num_popped;
}

/// Checks and enables features. It aborts if any of the features is not
/// supported.
static void negotiate_feature(uint64_t features) {
    features |= VIRTIO_F_VERSION_1;

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

struct virtio_ops virtio_modern_ops = {
    .read_device_features = read_device_features,
    .negotiate_feature = negotiate_feature,
    .read_device_config = read_device_config,
    .activate = activate,
    .read_isr_status = read_isr_status,
    .virtq_init = virtq_init,
    .virtq_get = virtq_get,
    .virtq_push = virtq_push,
    .virtq_pop = virtq_pop,
    .virtq_notify = virtq_notify,
};

static uint16_t get_transitional_device_id(int device_type) {
    switch (device_type) {
        case VIRTIO_DEVICE_NET:
            return 0x1000;
        default:
            return 0;
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
            default:
                return err;
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
        TRACE(
            "missing a BAR for the device access, falling back to the legacy driver....");
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
