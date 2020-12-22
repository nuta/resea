#include "pci.h"
#include "guest.h"
#include <resea/malloc.h>
#include <list.h>

static struct pci_device *lookup_device_by_slot(struct guest *guest, int slot) {
    LIST_FOR_EACH (device, &guest->pci.devices, struct pci_device, next) {
        if (device->slot == slot) {
            return device;
        }
    }

    return NULL;
}

uint32_t pci_handle_io_read(struct guest *guest, uint64_t port, size_t size) {
    switch (port) {
        // PCI Address
        case 0xcf8:
            if (guest->pci.selected_addr == 0x80000000) {
                // A dirty workaround for PCI bus probe in Linux.
                return 0x80000000;
            }

            return guest->pci.selected_addr;
        // PCI Data
        case 0xcfc ... 0xcff: {
            uint8_t bus = (guest->pci.selected_addr >> 16) & 0xff;
            uint8_t slot = (guest->pci.selected_addr >> 11) & 0x3f;
            uint8_t offset = (port - 0xcfc) + (guest->pci.selected_addr & 0xff);

            if (bus != 0) {
                return 0;
            }

            struct pci_device *device = lookup_device_by_slot(guest, slot);
            if (!device) {
                return 0;
            }

            if (guest->pci.read_bar0_size) {
                guest->pci.read_bar0_size = false;
                return device->port_len;
            }

            switch (size) {
                case 1: return ((uint8_t *) device->config)[offset];
                case 2: return ((uint16_t *) device->config)[offset / 2];
                case 4: return ((uint32_t *) device->config)[offset / 4];
            }

            UNREACHABLE();
        }
        default:
            UNREACHABLE();
    }
}

void pci_handle_io_write(struct guest *guest, uint64_t port, size_t size,
                         uint32_t value) {
    switch (port) {
        // PCI Address
        case 0xcf8:
            guest->pci.selected_addr = value;
            break;
        // PCI Data
        case 0xcfc:
            // TODO: check address
            if (value == 0xffffffff) {
                guest->pci.read_bar0_size = true;
            }
            break;
        default:
            WARN_DBG("pci: unhandled io write: port=%x", port);
    }
}

void pci_add_device(struct guest *guest, const char *name,
                    struct pci_device_ops *ops, struct pci_config *config,
                    void *data, size_t port_len, bool use_irq) {
    struct pci_device *device = malloc(sizeof(*device));
    device->guest = guest;
    device->name = name;
    device->ops = ops;
    device->config = config;
    device->port_base = guest->pci.next_port_base;
    device->port_len = port_len;
    device->slot = guest->pci.next_slot;
    device->data = data;

    if (port_len > 0) {
        config->bar0 = device->port_base | 1;
    }

    if (use_irq) {
        ASSERT(guest->pci.next_irq < 16);
        uint8_t irq = guest->pci.next_irq++;
        device->irq = irq;
        config->interrupt_line = irq;
        config->interrupt_pin = 1; // INTA#
    } else {
        device->irq = 0;
    }

    list_push_back(&guest->pci.devices, &device->next);
    guest->pci.next_port_base += ALIGN_UP(port_len, PAGE_SIZE);
    guest->pci.next_slot++;
}

static uint32_t host_bridge_ioport_read(struct pci_device *device, offset_t off,
                                        size_t size) {
    // No-op
    return 0;
}

static void host_bridge_ioport_write(struct pci_device *device, offset_t off,
                                     size_t size, uint32_t value) {
    // No-op
}

static struct pci_device_ops host_bridge_ops ={
    .ioport_read = host_bridge_ioport_read,
    .ioport_write = host_bridge_ioport_write,
};

static struct pci_config host_bridge_config = {
    .header_type = 0,
    .class = 0x06,    // Bridge Device
    .subclass = 0x00, // Host Bridge
};

void pci_init(struct guest *guest) {
    list_init(&guest->pci.devices);
    guest->pci.next_slot = 0;
    guest->pci.next_port_base = 0xa000;
    guest->pci.next_irq = 12; // IRQ #0 is used for PIT (timer)
    guest->pci.selected_addr = 0;
    guest->pci.read_bar0_size = false;

    pci_add_device(guest, "host-bridge", &host_bridge_ops, &host_bridge_config,
                   NULL, 0, false);
}
