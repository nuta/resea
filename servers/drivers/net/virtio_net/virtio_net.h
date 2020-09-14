#ifndef __VIRTIO_NET_H__
#define __VIRTIO_NET_H__

#include <types.h>

#define VIRTIO_NET_F_MAC      (1 << 5)
#define VIRTIO_NET_F_STATUS   (1 << 16)
#define VIRTIO_NET_QUEUE_RX   0
#define VIRTIO_NET_QUEUE_TX   1

struct virtio_net_config {
    uint8_t mac0;
    uint8_t mac1;
    uint8_t mac2;
    uint8_t mac3;
    uint8_t mac4;
    uint8_t mac5;
    uint16_t status;
    uint16_t max_virtqueue_pairs;
    uint16_t mtu;
} __packed;

#define VIRTIO_NET_HDR_GSO_NONE 0
struct virtio_net_header {
    uint8_t flags;
    uint8_t gso_type;
    uint16_t len;
    uint16_t gso_size;
    uint16_t checksum_start;
    uint16_t checksum_offset;
    uint16_t num_buffers;
} __packed;

struct virtio_net_buffer {
    struct virtio_net_header header;
    uint8_t payload[PAGE_SIZE - sizeof(struct virtio_net_header)];
} __packed;

#endif
