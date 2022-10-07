#pragma once
#include <types.h>

//
//  "5 Device Types"
//
#define VIRTIO_DEVICE_NET 1

//
//  "2.1 Device Status Field"
//
#define VIRTIO_STATUS_ACK       1
#define VIRTIO_STATUS_DRIVER    2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEAT_OK   82

#define VIRTQ_DESC_F_NEXT          1
#define VIRTQ_DESC_F_WRITE         2
#define VIRTQ_DESC_F_AVAIL_SHIFT   7
#define VIRTQ_DESC_F_USED_SHIFT    15
#define VIRTQ_DESC_F_AVAIL         (1 << VIRTQ_DESC_F_AVAIL_SHIFT)
#define VIRTQ_DESC_F_USED          (1 << VIRTQ_DESC_F_USED_SHIFT)
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

/// A virtqueue.
struct virtio_virtq {
    /// The virtqueue index.
    unsigned index;
    /// The number of descriptors.
    int num_descs;
};

#define VIRTIO_REG_DEVICE_FEATS       0x00
#define VIRTIO_REG_DRIVER_FEATS       0x04
#define VIRTIO_REG_QUEUE_ADDR_PFN     0x08
#define VIRTIO_REG_NUM_DESCS          0x0c
#define VIRTIO_REG_QUEUE_SELECT       0x0e
#define VIRTIO_REG_QUEUE_NOTIFY       0x10
#define VIRTIO_REG_DEVICE_STATUS      0x12
#define VIRTIO_REG_ISR_STATUS         0x13
#define VIRTIO_REG_DEVICE_CONFIG_BASE 0x14

#define DEVICE_STATUS_ACKNOWLEDGE        1
#define DEVICE_STATUS_DRIVER             2
#define DEVICE_STATUS_DRIVER_OK          4
#define DEVICE_STATUS_FEATURES_OK        8
#define DEVICE_STATUS_DEVICE_NEEDS_RESET 64
#define DEVICE_STATUS_FAILED             128

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __packed;

struct virtq_avail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[];
} __packed;

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __packed;

struct virtq_used {
    uint16_t flags;
    uint16_t index;
    struct virtq_used_elem ring[];
} __packed;
