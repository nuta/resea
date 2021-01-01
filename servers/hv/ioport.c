#include "ioport.h"
#include "guest.h"
#include "mm.h"
#include "pci.h"
#include <resea/printf.h>

uint32_t handle_io_read(struct guest *guest, uint64_t port, size_t size) {
    switch (port) {
        case 0xcf8 ... 0xcff:
            return pci_handle_io_read(guest, port, size);
        default:
            LIST_FOR_EACH (device, &guest->pci.devices, struct pci_device,
                           next) {
                if (device->port_base <= port
                    && port < device->port_base + device->port_len) {
                    return device->ops->ioport_read(
                        device, port - device->port_base, size);
                }
            }
    }

    WARN_DBG("unsupported port read access: port=%x", port);
    return 0;
}

void handle_io_write(struct guest *guest, uint64_t port, size_t size,
                     uint32_t value) {
    switch (port) {
        case 0xcf8 ... 0xcff:
            pci_handle_io_write(guest, port, size, value);
            return;
        default:
            LIST_FOR_EACH (device, &guest->pci.devices, struct pci_device,
                           next) {
                if (device->port_base <= port
                    && port < device->port_base + device->port_len) {
                    device->ops->ioport_write(device, port - device->port_base,
                                              size, value);
                    return;
                }
            }
    }

    WARN_DBG("unsupported port write access: port=%x", port);
}
