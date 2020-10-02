#ifndef __VIRTIO_VIRTIO_MODERN_H__
#define __VIRTIO_VIRTIO_MODERN_H__

#include <types.h>
#include <driver/io.h>
#include <driver/dma.h>

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

struct virtq_event_suppress {
    uint16_t desc;
    uint16_t flags;
} __packed;

struct virtq_packed_desc {
    /// The physical buffer address.
    uint64_t addr;
    /// The buffer Length.
    uint32_t len;
    /// The buffer ID.
    uint16_t id;
    /// Flags.
    uint16_t flags;
} __packed;

struct virtio_virtq_modern {
    struct virtq_packed_desc *descs;
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

struct virtio_ops;
error_t virtio_modern_find_device(int device_type, struct virtio_ops **ops, uint8_t *irq);

#endif
