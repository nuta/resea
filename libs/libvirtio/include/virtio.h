#ifndef __VIRTIO_H__
#define __VIRTIO_H__

#include <types.h>

#define VIRTIO_PCI_VENDOR 0x1af4
#define VIRTIO_PCI_DEVICE 0xffff

#define VIRTQ_DESC_F_NEXT      1
#define VIRTQ_DESC_F_WRITE     2
#define VIRTQ_DESC_F_INDIRECT  4

struct virtq_desc {
    uint64_t paddr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} PACKED;

#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

struct virtq_avail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[];
    // We don't use `used_event` field defined in the spec.
} PACKED;

struct virtq_used_elem {
    /// The index of start of the used descriptor chain.
    uint32_t id;
    /// The number of descriptors in the chain.
    uint32_t len;
} PACKED;

struct virtq_used {
    uint16_t flags;
    uint16_t index;
    struct virtq_used_elem ring[];
    // We don't use `avail_event` field defined in the spec.
} PACKED;

struct virtq {
    size_t queue_size;
    paddr_t paddr;
    uint64_t buffer;
    struct virtq_desc *desc;
    struct virtq_avail *avail;
    struct virtq_used *used;
};

#define VIRTIO_PCI_CAP_CFG_TYPE   3
#define VIRTIO_PCI_CAP_BAR        4
#define VIRTIO_PCI_CAP_OFFSET     8
#define VIRTIO_PCI_CAP_LEN        12

struct virtio_pci_cap {
    uint8_t cap_vendor;
    uint8_t cap_next;
    uint8_t cap_len;
    uint8_t cfg_type;
    uint8_t bar;
    uint8_t padding[3];
    uint32_t offset;
    uint32_t length;
} PACKED;

#define VIRTIO_PCI_CAP_VENDOR  0x09
#define VIRTIO_PCI_CAP_COMMON_CFG 1
struct virtio_pci_common_cfg {
    uint32_t device_feature_select;
    uint32_t device_feature;
    uint32_t driver_feature_select;
    uint32_t driver_feature;
    uint16_t msix_config;
    uint16_t num_queues;
    uint8_t  device_status;
    uint8_t  config_generation;
    uint16_t queue_select;
    uint16_t queue_size;
    uint16_t queue_msix_vector;
    uint16_t queue_enable;
    uint16_t queue_notify_off;
    uint64_t queue_desc;
    uint64_t queue_driver;
    uint64_t queue_device;
} PACKED;

#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
struct virtio_pci_notify_cap {
    struct virtio_pci_cap cap;
    uint32_t notify_off_mul;
} PACKED;

#define NUM_VIRTQ_MAX 4
struct virtio_pci {
    /// The pci server.
    cid_t server;
    uint8_t bus;
    uint8_t slot;
    uint16_t *notify_addrs[NUM_VIRTQ_MAX];
    struct virtio_pci_common_cfg *common_cfg;
};

struct virtio_device {
    cid_t memmgr;
    struct virtio_pci pci;
    struct virtq virtq[NUM_VIRTQ_MAX];
};

struct virtio_request {
    uint64_t paddr;
    uint32_t len;
    uint16_t flags;
    struct virtio_request *next;
};

error_t virtio_finish_init(struct virtio_device *device);
error_t virtio_setup_virtq(struct virtio_device *device, uint8_t index);
error_t virtio_setup_pci_device(struct virtio_device *device,
                                cid_t memmgr, cid_t pci_server);

#endif
