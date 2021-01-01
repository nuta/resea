#include "virtio_modern.h"
#include <driver/io.h>
#include <endian.h>
#include <virtio/virtio.h>

error_t virtio_find_device(int device_type, struct virtio_ops **ops,
                           uint8_t *irq) {
    if (virtio_modern_find_device(device_type, ops, irq) == OK) {
        return OK;
    }

    return virtio_legacy_find_device(device_type, ops, irq);
}
