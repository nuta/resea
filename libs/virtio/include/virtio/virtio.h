#ifndef __VIRTIO_VIRTIO_H__
#define __VIRTIO_VIRTIO_H__

#include "../virtio_legacy.h"
#include "../virtio_modern.h"
#include <driver/dma.h>
#include <types.h>

//
//  "5 Device Types"
//
#define VIRTIO_DEVICE_NET 1
#define VIRTIO_DEVICE_GPU 16

//
//  "2.1 Device Status Field"
//
#define VIRTIO_STATUS_ACK       1
#define VIRTIO_STATUS_DRIVER    2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTIO_STATUS_FEAT_OK   82

#define VIRTIO_F_VERSION_1   (1ull << 32)
#define VIRTIO_F_RING_PACKED (1ull << 34)

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
    /// Descriptors.
    dma_t descs_dma;
    /// The number of descriptors.
    int num_descs;
    /// Static buffers referenced from descriptors.
    dma_t buffers_dma;
    void *buffers;
    /// The size of a buffer entry.
    size_t buffer_size;
    /// The next descriptor indioces. -1 if the next descriptor does not exist.
    int *next_indices;
    union {
        struct virtio_virtq_legacy legacy;
        struct virtio_virtq_modern modern;
    };
};

struct virtio_chain_entry {
    paddr_t addr;
    uint32_t len;
    bool device_writable;
};

#define VIRTQ_ALLOC_NO_PREV -1

struct virtio_ops {
    uint64_t (*read_device_features)(void);
    void (*negotiate_feature)(uint64_t features);
    uint64_t (*read_device_config)(offset_t offset, size_t size);
    void (*activate)(void);
    uint8_t (*read_isr_status)(void);
    void (*virtq_init)(unsigned index);
    struct virtio_virtq *(*virtq_get)(unsigned index);
    error_t (*virtq_push)(struct virtio_virtq *vq,
                          struct virtio_chain_entry *chain, int n);
    int (*virtq_pop)(struct virtio_virtq *vq, struct virtio_chain_entry *chain,
                     int n, size_t *total_len);
    void (*virtq_notify)(struct virtio_virtq *vq);
};

error_t virtio_find_device(int device_type, struct virtio_ops **ops,
                           uint8_t *irq);

#endif
