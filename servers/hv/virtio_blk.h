#ifndef __VIRTIO_BLK_H__
#define __VIRTIO_BLK_H__

#include <types.h>

#define SECTOR_SIZE             512
#define NUM_VIRTQUEUES_MAX      4
#define VIRITO_BLK_REG_CAPACITY 0

#define VIRTIO_BLK_T_IN     0
#define VIRTIO_BLK_T_OUT    1
#define VIRTIO_BLK_T_FLUSH  4
#define VIRTIO_BLK_S_OK     0
#define VIRTIO_BLK_S_IOERR  1
#define VIRTIO_BLK_S_UNSUPP 2

struct __packed virtio_blk_req_header {
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;
    uint8_t data[];
    // The status field (uint8_t) comes right after data!
};

struct virtqueue {
    gpaddr_t gpaddr;
    size_t num_descs;
    unsigned avail_index;
    unsigned used_index;
};

struct virtio_blk_device {
    task_t blk_device_server;
    unsigned selected_virtq;
    bool selected_nonexistent_virtq;
    unsigned num_virtqueues;
    bool sending_interrupt;
    struct virtqueue virtqueues[NUM_VIRTQUEUES_MAX];
};

struct guest;
void virtio_blk_init(struct guest *guest);

#endif
