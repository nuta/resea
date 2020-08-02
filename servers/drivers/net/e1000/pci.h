#ifndef __PCI_H__
#define __PCI_H__

#include <types.h>

#define PCI_IOPORT_ADDR 0x0cf8
#define PCI_IOPORT_DATA 0x0cfc
#define PCI_ANY         0

#define PCI_CONFIG_VENDOR_ID 0x00
#define PCI_CONFIG_DEVICE_ID 0x02
#define PCI_CONFIG_COMMAND   0x04
#define PCI_CONFIG_BAR0      0x10
#define PCI_CONFIG_INTR_LINE 0x3c

struct pci_device {
    uint8_t bus;
    uint8_t slot;
    uint16_t vendor;
    uint16_t device;
    uint32_t bar0;
    uint8_t irq;
};

void pci_enable_bus_master(struct pci_device *dev);
bool pci_find_device(struct pci_device *dev, uint16_t vendor, uint16_t device);

#endif
