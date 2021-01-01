#include "virtio_blk.h"
#include "guest.h"
#include "ioport.h"
#include "mm.h"
#include "pci.h"
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>
#include <virtio/virtio.h>

static void handle_pending_requests(struct pci_device *device) {
    struct virtio_blk_device *virtio =
        (struct virtio_blk_device *) device->data;
    struct virtqueue *vq = &virtio->virtqueues[virtio->selected_virtq];

    offset_t avail_ring_off = sizeof(struct virtq_desc) * vq->num_descs;
    size_t avail_ring_size = sizeof(uint16_t) * (3 + vq->num_descs);
    offset_t used_ring_off =
        ALIGN_UP(avail_ring_off + avail_ring_size, PAGE_SIZE);

    // XXX: What if descriptors span across multiple pages?
    struct virtq_desc *descs = (struct virtq_desc *) resolve_guest_paddr_buffer(
        device->guest, vq->gpaddr);
    if (!descs) {
        WARN_DBG("virtio-blk: descriptors are not accessible");
        return;
    }

    // XXX: What if the ring spans across multiple pages?
    struct virtq_avail *avail_ring =
        (struct virtq_avail *) resolve_guest_paddr_buffer(
            device->guest, vq->gpaddr + avail_ring_off);
    if (!avail_ring) {
        WARN_DBG("virtio-blk: avail ring is not accessible");
        return;
    }

    // XXX: What if the ring spans across multiple pages?
    struct virtq_used *used_ring =
        (struct virtq_used *) resolve_guest_paddr_buffer(
            device->guest, vq->gpaddr + used_ring_off);
    if (!used_ring) {
        WARN_DBG("virtio-blk: used ring is not accessible");
        return;
    }

    while (vq->avail_index != avail_ring->index) {
        unsigned head_desc_index = avail_ring->ring[vq->avail_index];
        size_t written_len = 0;

        unsigned desc_index = head_desc_index;
        struct virtio_blk_req_header *req = NULL;
        offset_t sector = 0xffffff;
        size_t remaining;
        while (true) {
            struct virtq_desc *desc = &descs[desc_index];
            bool writable = (desc->flags & VIRTQ_DESC_F_WRITE) != 0;

            void *ptr = resolve_guest_paddr_buffer(device->guest, desc->addr);
            if (!ptr) {
                WARN_DBG("virtio-blk: descriptor data is not accessible");
                return;
            }

            uint8_t *data = NULL;
            if (!req) {
                req = (struct virtio_blk_req_header *) ptr;
                sector = req->sector;
                remaining = desc->len - sizeof(*req);

                if (desc->len < sizeof(*req)) {
                    WARN_DBG("virtio-blk: too short request (desc->len=%d)",
                             desc->len);
                    return;
                }

                if (desc->len > sizeof(*req)) {
                    data = req->data;
                }
            } else {
                data = ptr;
                remaining = desc->len;
            }

            if (data) {
                switch (req->type) {
                    case VIRTIO_BLK_T_IN:
                        if (!writable) {
                            return;
                        }

                        size_t num_sectors =
                            ALIGN_DOWN(remaining, SECTOR_SIZE) / SECTOR_SIZE;
                        if (num_sectors > 0) {
                            struct message m;
                            m.type = BLK_READ_MSG;
                            m.blk_read.sector = sector;
                            m.blk_read.num_sectors = num_sectors;
                            ASSERT_OK(ipc_call(virtio->blk_device_server, &m));
                            ASSERT(m.type == BLK_READ_REPLY_MSG);

                            memcpy(data, m.blk_read_reply.data,
                                   m.blk_read_reply.data_len);

                            remaining -= num_sectors * SECTOR_SIZE;
                            written_len += num_sectors * SECTOR_SIZE;
                        }

                        if (remaining == 1) {
                            uint8_t *status = &data[num_sectors * SECTOR_SIZE];
                            *status = VIRTIO_BLK_S_OK;
                            written_len++;
                        }
                        break;
                    case VIRTIO_BLK_T_OUT: {
                        size_t num_sectors =
                            ALIGN_DOWN(remaining, SECTOR_SIZE) / SECTOR_SIZE;
                        if (num_sectors > 0) {
                            struct message m;
                            m.type = BLK_WRITE_MSG;
                            m.blk_write.sector = sector;
                            m.blk_write.data = data;
                            m.blk_write.data_len = num_sectors * SECTOR_SIZE;
                            ASSERT_OK(ipc_call(virtio->blk_device_server, &m));
                            ASSERT(m.type == BLK_WRITE_REPLY_MSG);

                            remaining -= num_sectors * SECTOR_SIZE;
                            written_len += num_sectors * SECTOR_SIZE;
                        }

                        if (remaining == 1 && writable) {
                            uint8_t *status = &data[num_sectors * SECTOR_SIZE];
                            *status = VIRTIO_BLK_S_OK;
                            written_len++;
                        }
                        break;
                    }
                    default:
                        PANIC("NYI");
                }
            }

            if ((desc->flags & VIRTQ_DESC_F_NEXT) == 0) {
                break;
            }

            desc_index = desc->next;
        }

        used_ring->ring[used_ring->index].id = head_desc_index;
        used_ring->ring[used_ring->index].len = written_len;
        used_ring->index = (used_ring->index + 1) % vq->num_descs;
        vq->avail_index = (vq->avail_index + 1) % vq->num_descs;
    }

    if ((avail_ring->flags & VIRTQ_AVAIL_F_NO_INTERRUPT) == 0) {
        // Inject an interrupt.
        guest_inject_irq(device->guest, device->irq);
        virtio->sending_interrupt = true;
    }
}

static uint32_t ioport_read(struct pci_device *device, offset_t off,
                            size_t size) {
    struct virtio_blk_device *virtio =
        (struct virtio_blk_device *) device->data;
    switch (off) {
        case VIRTIO_REG_DEVICE_FEATS:
            return 0;
        case VIRTIO_REG_DRIVER_FEATS:
            return 0;
        case VIRTIO_REG_QUEUE_ADDR_PFN:
            return virtio->virtqueues[virtio->selected_virtq].gpaddr;
        case VIRTIO_REG_NUM_DESCS:
            if (virtio->selected_nonexistent_virtq) {
                // THe selected virtqueue does not exist.
                return 0;
            }

            return virtio->virtqueues[virtio->selected_virtq].num_descs;
        case VIRTIO_REG_QUEUE_SELECT:
            return virtio->selected_virtq;
        case VIRTIO_REG_QUEUE_NOTIFY:
            return 0;
        case VIRTIO_REG_DEVICE_STATUS:
            return DEVICE_STATUS_FEATURES_OK | DEVICE_STATUS_ACKNOWLEDGE;
        case VIRTIO_REG_ISR_STATUS: {
            int value = virtio->sending_interrupt ? 1 : 0;
            virtio->sending_interrupt = false;
            return value;
        }
        case VIRTIO_REG_DEVICE_CONFIG_BASE
            + VIRITO_BLK_REG_CAPACITY... VIRTIO_REG_DEVICE_CONFIG_BASE
            + VIRITO_BLK_REG_CAPACITY + sizeof(uint64_t): {
            uint64_t value = (256 * 1024) / SECTOR_SIZE;  // FIXME:
            int access_off =
                off - VIRTIO_REG_DEVICE_CONFIG_BASE + VIRITO_BLK_REG_CAPACITY;
            switch (size) {
                case 1:
                    return (value >> (8 * access_off)) & 0xff;
                case 2:
                    return (value >> (8 * access_off)) & 0xffff;
                case 4:
                    return (value >> (8 * access_off)) & 0xffffffff;
                default:
                    UNREACHABLE();
            }
        }
        default:
            WARN_DBG("virtio-blk: unknown register access: off=%x", off);
            return 0;
    }
}

static void ioport_write(struct pci_device *device, offset_t off, size_t size,
                         uint32_t value) {
    struct virtio_blk_device *virtio =
        (struct virtio_blk_device *) device->data;
    switch (off) {
        case VIRTIO_REG_DEVICE_FEATS:
            break;
        case VIRTIO_REG_DRIVER_FEATS:
            break;
        case VIRTIO_REG_QUEUE_ADDR_PFN:
            virtio->virtqueues[virtio->selected_virtq].gpaddr =
                value * PAGE_SIZE;
            break;
        case VIRTIO_REG_NUM_DESCS:
            if (value == 0) {
                break;
            }

            virtio->virtqueues[virtio->selected_virtq].num_descs = value;
            break;
        case VIRTIO_REG_QUEUE_SELECT:
            if (value > virtio->num_virtqueues) {
                virtio->selected_nonexistent_virtq = true;
                break;
            }

            virtio->selected_virtq = value;
            virtio->selected_nonexistent_virtq = false;
            break;
        case VIRTIO_REG_QUEUE_NOTIFY:
            handle_pending_requests(device);
            break;
        case VIRTIO_REG_DEVICE_STATUS:
            break;
        case VIRTIO_REG_ISR_STATUS:
            break;
        case VIRTIO_REG_DEVICE_CONFIG_BASE + VIRITO_BLK_REG_CAPACITY:
            break;
        default:
            WARN_DBG("virtio-blk: unknown register access: off=%x", off);
    }
}

static struct pci_config config = {
    .header_type = 0,
    .vendor_id = 0x1af4,
    .device_id = 0x1001,
    .subsystem_id = 0x02, /* block device */
    .class = 0x02,        // Network Controller
    .subclass = 0x00,     // Ethernet Controller
    .bar0 = 0xa001,       /* IO port, base=0xa000 */
};

static struct pci_device_ops ops = {
    .ioport_read = ioport_read,
    .ioport_write = ioport_write,
};

void virtio_blk_init(struct guest *guest) {
    struct virtio_blk_device *virtio = malloc(sizeof(*virtio));
    virtio->blk_device_server = ipc_lookup("disk");
    virtio->selected_virtq = 0;
    virtio->selected_nonexistent_virtq = false;
    virtio->num_virtqueues = 1;
    virtio->sending_interrupt = false;
    virtio->virtqueues[0].gpaddr = 0;
    virtio->virtqueues[0].num_descs = 32;
    virtio->virtqueues[0].avail_index = 0;
    virtio->virtqueues[0].used_index = 0;
    pci_add_device(guest, "virtio-blk", &ops, &config, virtio, 0x1000, true);
}
