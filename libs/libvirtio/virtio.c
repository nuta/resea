#include <resea.h>
#include <resea/math.h>
#include <driver/pci.h>
#include <idl_stubs.h>
#include <virtio.h>

static error_t read_pci_config8(struct virtio_device *device,
                                uint16_t offset, uint8_t *data) {
    uint16_t data16;
    TRY(call_pci_read_config16(device->pci.server, device->pci.bus,
                               device->pci.slot, offset, &data16));
    *data = data16 & 0xff;
    return OK;
}

static error_t read_pci_config16(struct virtio_device *device,
                                 uint16_t offset, uint16_t *data) {
    return call_pci_read_config16(device->pci.server, device->pci.bus,
                                  device->pci.slot, offset, data);
}

static error_t read_pci_config32(struct virtio_device *device,
                                 uint16_t offset, uint32_t *data) {
    return call_pci_read_config32(device->pci.server, device->pci.bus,
                                  device->pci.slot, offset, data);
}

error_t virtio_setup_virtq(struct virtio_device *device, uint8_t index) {
    assert(index < NUM_VIRTQ_MAX);
    struct virtq *virtq = &device->virtq[index];
    struct virtio_pci_common_cfg *common = device->pci.common_cfg;

    common->queue_select = index;
    __sync_synchronize();

    int queue_size = common->queue_size;
    int qalign = 8; /* TODO: I'm not sure the correct value for this. */

    size_t desc_size = sizeof(struct virtq_desc) * queue_size;
    size_t avail_size = sizeof(struct virtq_avail)
                        + sizeof(uint16_t) * queue_size
                        + sizeof(uint16_t) /* avail_event */;
    size_t used_size = sizeof(struct virtq_used)
                       + sizeof(uint16_t) * queue_size
                       + sizeof(uint16_t) /* used_event */;
    size_t virtq_size =
        ALIGN_UP(desc_size + avail_size, qalign) + ALIGN_UP(used_size, qalign);
    size_t desc_offset  = 0;
    size_t avail_offset = desc_size;
    size_t used_offset  = ALIGN_UP(desc_size + avail_size, qalign);

    page_base_t page_base = valloc(virtq_size);
    int order = 4; /* FIXME: */
    uintptr_t buffer;
    size_t num_pages;
    paddr_t virtq_paddr;
    TRY(call_memory_alloc_phy_pages(device->memmgr, 0, order,
                                    page_base, &buffer, &num_pages,
                                    &virtq_paddr));

    virtq->queue_size = queue_size;
    virtq->buffer = buffer;
    virtq->paddr = virtq_paddr;
    virtq->desc  = (struct virtq_desc *)  (buffer + desc_offset);
    virtq->avail = (struct virtq_avail *) (buffer + avail_offset);
    virtq->used  = (struct virtq_used *)  (buffer + used_offset);

    common->queue_desc   = virtq_paddr + desc_offset;
    common->queue_driver = virtq_paddr + avail_offset;
    common->queue_device = virtq_paddr + used_offset;
    common->queue_enable = 1;
    __sync_synchronize();

    return OK;
}

static error_t get_pci_capability_offset(struct virtio_device *device,
                                         uint8_t cap_vendor, uint8_t cfg_type,
                                         uint8_t *cap_offset) {
    uint8_t offset;
    TRY(read_pci_config8(device, PCI_CONFIG_CAPABILITIES, &offset));
    offset = offset & 0xfc;
    while (offset) {
        uint8_t vendor;
        uint8_t type;
        uint8_t next;
        TRY(read_pci_config8(device, offset + PCI_CAP_VENDOR, &vendor));
        if (vendor == cap_vendor) {
            TRY(read_pci_config8(device, offset + VIRTIO_PCI_CAP_CFG_TYPE,
                                 &type));
            if (type == cfg_type) {
                *cap_offset = offset;
                return OK;
            }
        }

        TRY(read_pci_config8(device, offset + PCI_CAP_NEXT, &next));
        INFO("vendor = %x, cfg_type =%d, cap_next = %d", vendor, type, next);
        offset = next;
    }

    *cap_offset = 0;
    return ERR_NOT_FOUND;
}

static error_t map_pci_config_cfg(struct virtio_device *device) {
    // Make sure that the virtio device supports capabilities list.
    uint16_t status;
    TRY(read_pci_config16(device, PCI_CONFIG_STATUS, &status));
    if ((status & PCI_CONFIG_STATUS_CAPS) == 0) {
        WARN("The virtio device (%x:%x) does not support capability list.",
             device->pci.bus, device->pci.slot);
        return ERR_NOT_ACCEPTABLE;
    }

    uint8_t common_off;
    TRY(get_pci_capability_offset(device, VIRTIO_PCI_CAP_VENDOR,
                                  VIRTIO_PCI_CAP_COMMON_CFG, &common_off));

    // Map the common cfg.
    uint8_t bar_no;
    TRY(read_pci_config8(device, common_off + VIRTIO_PCI_CAP_BAR,
                         &bar_no));
    assert(bar_no <= 5);
    uint32_t bar;
    TRY(read_pci_config32(device, PCI_CONFIG_NTH_BAR(bar_no), &bar));
    assert(PCI_BAR_IS_MEMORY_MAPPED(bar));
    INFO("bar = %p, bar_no = %d", bar, bar_no);


    size_t bar_size = 4096; /* FIXME: Is this large enough? */
    page_base_t page_base = valloc(bar_size);
    uintptr_t bar_buffer;
    size_t num_pages;
    paddr_t bar_paddr;
    TRY(call_memory_alloc_phy_pages(device->memmgr,
                                    PCI_BAR_ADDR(bar) /* map at */, 0,
                                    page_base, &bar_buffer, &num_pages,
                                    &bar_paddr));

    uint32_t common_cfg_off;
    TRY(read_pci_config32(device, common_off + VIRTIO_PCI_CAP_OFFSET,
                          &common_cfg_off));

    INFO("common_cfg_off = %d", common_cfg_off);
    struct virtio_pci_common_cfg *common_cfg =
        (struct virtio_pci_common_cfg *) (bar_buffer + common_cfg_off);

    common_cfg->queue_select = 0;
    __sync_synchronize();
    INFO("common_cfg: qsz = %d", common_cfg->queue_size);

    device->pci.common_cfg = common_cfg;

    //
    //  Determine Queue Notify addresses
    //

    uint8_t notify_cap_off;
    TRY(get_pci_capability_offset(device, VIRTIO_PCI_CAP_VENDOR,
                                  VIRTIO_PCI_CAP_NOTIFY_CFG, &notify_cap_off));

    uint32_t notify_off_mul;
    TRY(read_pci_config32(device,
        notify_cap_off + offsetof(struct virtio_pci_notify_cap, notify_off_mul),
        &notify_off_mul));

    uint8_t notify_bar_no;
    TRY(read_pci_config8(device, notify_cap_off + VIRTIO_PCI_CAP_BAR,
                         &notify_bar_no));
    assert(bar_no == notify_bar_no);

    uint32_t notify_addr_off;
    TRY(read_pci_config32(device, common_off + VIRTIO_PCI_CAP_OFFSET,
                          &notify_addr_off));

    for (int queue_index = 0; queue_index < NUM_VIRTQ_MAX; queue_index++) {
        device->pci.notify_addrs[queue_index] =
            (uint16_t *) (bar_buffer + notify_addr_off + device->pci.common_cfg->queue_notify_off * notify_off_mul);
    }
    return OK;
}

#define VIRTIO_STATUS_ACKNOWLEDGE  1
#define VIRTIO_STATUS_DRIVER       2
#define VIRTIO_STATUS_FEATURES_OK  8
#define VIRTIO_STATUS_DRIVER_OK    4
static error_t negotiate_features_over_pci(struct virtio_device *device) {
    // Reset the device.
    device->pci.common_cfg->device_status = 0;
    __sync_synchronize();

    // Tell the virtio device that we know how to use virtio.
    device->pci.common_cfg->device_status |= VIRTIO_STATUS_ACKNOWLEDGE;
    __sync_synchronize();
    device->pci.common_cfg->device_status |= VIRTIO_STATUS_DRIVER;
    __sync_synchronize();

    // TODO: feature negotiation
    device->pci.common_cfg->device_status |= VIRTIO_STATUS_FEATURES_OK;
    __sync_synchronize();
    return OK;
}

error_t setup_pci_device(struct virtio_device *device) {
    TRY(map_pci_config_cfg(device));
    TRY(negotiate_features_over_pci(device));
    return OK;
}

error_t virtio_finish_init(struct virtio_device *device) {
    device->pci.common_cfg->device_status |= VIRTIO_STATUS_DRIVER_OK;
    __sync_synchronize();
    return OK;
}

error_t virtio_setup_pci_device(struct virtio_device *device,
                                cid_t memmgr, cid_t pci_server) {
    uint16_t subvendor, subdevice;
    uint8_t bus, slot;
    TRY(call_pci_wait_for_device(pci_server, VIRTIO_PCI_VENDOR,
                                 VIRTIO_PCI_DEVICE, &subvendor, &subdevice,
                                 &bus, &slot));

    // TODO: Check the device type.

    TRACE("virtio: Found a virtio device: bus=%d, slot=%d", bus, slot);

    device->memmgr = memmgr;
    device->pci.server = pci_server;
    device->pci.bus = bus;
    device->pci.slot = slot;

    TRY(setup_pci_device(device));
    return OK;
}

/*
static error_t virtio_add_request(struct virtio_device *device,
                                  struct virtio_request *req) {
    // TODO:
    return OK;
}
*/
