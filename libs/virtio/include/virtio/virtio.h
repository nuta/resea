#ifndef __VIRTIO_VIRTIO_H__
#define __VIRTIO_VIRTIO_H__

#include <resea/ipc.h>
#include <resea/async.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <driver/irq.h>
#include <driver/io.h>
#include <driver/dma.h>
#include <string.h>

extern io_t common_cfg_io;
extern offset_t common_cfg_off;
extern io_t device_cfg_io;
extern offset_t device_cfg_off;
extern io_t notify_struct_io;
extern offset_t notify_cap_off;
extern io_t isr_struct_io;
extern offset_t isr_cap_off;

//
//  PCI
//
#define VIRTIO_PCI_CAP_COMMON_CFG  1
#define VIRTIO_PCI_CAP_NOTIFY_CFG  2
#define VIRTIO_PCI_CAP_ISR_CFG     3
#define VIRTIO_PCI_CAP_DEVICE_CFG  4

struct virtio_pci_common_cfg {
    uint32_t device_feature_select;
    uint32_t device_feature;
    uint32_t driver_feature_select;
    uint32_t driver_feature;
    uint16_t msix_config;
    uint16_t num_queues;
    uint8_t device_status;
    uint8_t config_generation;
    uint16_t queue_select;
    uint16_t queue_size;
    uint16_t queue_msix_vector;
    uint16_t queue_enable;
    uint16_t queue_notify_off;
    uint32_t queue_desc_lo;
    uint32_t queue_desc_hi;
    uint32_t queue_driver_lo;
    uint32_t queue_driver_hi;
    uint32_t queue_device_lo;
    uint32_t queue_device_hi;
} __packed;

//
//  "5 Device Types"
//
#define VIRTIO_DEVICE_NET      1

//
//  "2.1 Device Status Field"
//
#define VIRTIO_STATUS_ACK        1
#define VIRTIO_STATUS_DRIVER     2
#define VIRTIO_STATUS_DRIVER_OK  4
#define VIRTIO_STATUS_FEAT_OK    82

#define VIRTIO_F_VERSION_1       (1ull << 32)
#define VIRTIO_F_RING_PACKED     (1ull << 34)

/// A virtqueue.
struct virtio_virtq {
    /// The virtqueue index.
    unsigned index;
    /// Descriptors.
    dma_t descs_dma;
    struct virtq_desc *descs;
    /// The number of descriptors.
    int num_descs;
    /// Static buffers referenced from descriptors.
    dma_t buffers_dma;
    void *buffers;
    /// The size of a buffer.
    size_t buffer_size;
    /// The queue notify offset for the queue.
    offset_t queue_notify_off;
    /// The next descriptor index to be allocated.
    int next_avail;
    /// The next descriptor index to be used by the device.
    int next_used;
    /// Driver-side wrapping counter.
    int avail_wrap_counter;
    /// Device-side wrapping counter.
    int used_wrap_counter;
};

#define VIRTQ_DESC_F_AVAIL_SHIFT  7
#define VIRTQ_DESC_F_USED_SHIFT   15
#define VIRTQ_DESC_F_AVAIL        (1 << VIRTQ_DESC_F_AVAIL_SHIFT)
#define VIRTQ_DESC_F_USED         (1 << VIRTQ_DESC_F_USED_SHIFT)
#define VIRTQ_DESC_F_WRITE        2

struct virtq_desc {
    /// The physical buffer address.
    uint64_t addr;
    /// The buffer Length.
    uint32_t len;
    /// The buffer ID.
    uint16_t id;
    /// Flags.
    uint16_t flags;
} __packed;

struct virtq_event_suppress {
    uint16_t desc;
    uint16_t flags;
} __packed;

#define _COMMON_CFG_READ(size, field) \
    io_read ## size(common_cfg_io, common_cfg_off + \
        offsetof(struct virtio_pci_common_cfg, field))
#define VIRTIO_COMMON_CFG_READ8(field) \
    _COMMON_CFG_READ(8, field)
#define VIRTIO_COMMON_CFG_READ16(field) \
    from_le16(_COMMON_CFG_READ(16, field))
#define VIRTIO_COMMON_CFG_READ32(field) \
    from_le32(_COMMON_CFG_READ(32, field))

#define _COMMON_CFG_WRITE(size, field, value) \
    io_write ## size(common_cfg_io, common_cfg_off + \
        offsetof(struct virtio_pci_common_cfg, field), value)
#define VIRTIO_COMMON_CFG_WRITE8(field, value) \
    _COMMON_CFG_WRITE(8, field, value)
#define VIRTIO_COMMON_CFG_WRITE16(field, value) \
    _COMMON_CFG_WRITE(16, field, into_le32(value))
#define VIRTIO_COMMON_CFG_WRITE32(field, value) \
    _COMMON_CFG_WRITE(32, field, into_le32(value))

#define _DEVICE_CFG_READ(size, struct_name, field) \
    io_read ## size(device_cfg_io, device_cfg_off + offsetof(struct_name, field))
#define VIRTIO_DEVICE_CFG_READ8(struct_name, field) \
    _DEVICE_CFG_READ(8, struct_name, field)
#define VIRTIO_DEVICE_CFG_READ16(struct_name, field) \
    from_le16(_DEVICE_CFG_READ(16, struct_name, field))
#define VIRTIO_DEVICE_CFG_READ32(struct_name, field) \
    from_le32(_DEVICE_CFG_READ(32, struct_name, field))

error_t virtio_pci_init(int device_type, uint8_t *irq);
uint64_t virtio_device_features(void);
void virtio_negotiate_feature(uint64_t features);
void virtio_init_virtqueues(void);
void virtio_activate(void);
uint8_t virtio_read_isr_status(void);

struct virtio_virtq *virtq_get(unsigned index);
uint16_t virtq_size(void);
void virtq_allocate_buffers(struct virtio_virtq *vq, size_t buffer_size,
                            bool writable);
int virtq_alloc(struct virtio_virtq *vq, size_t len);
struct virtq_desc *virtq_pop_desc(struct virtio_virtq *vq);
void virtq_push_desc(struct virtio_virtq *vq, struct virtq_desc *desc);
void virtq_notify(struct virtio_virtq *vq);

#endif
