#include <endian.h>
#include <driver/io.h>
#include <virtio/virtio.h>
#include "virtio_modern.h"

error_t virtio_find_device(int device_type, struct virtio_ops **ops, uint8_t *irq) {
    if (virtio_modern_find_device(device_type, ops, irq) == OK) {
        return OK;
    }

    return virtio_legacy_find_device(device_type, ops, irq);
}
