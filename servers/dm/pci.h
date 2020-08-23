#ifndef __PCI_H__
#define __PCI_H__

#include <types.h>

#define PCI_IOPORT_BASE 0x0cf8
#define PCI_IOPORT_ADDR 0x00
#define PCI_IOPORT_DATA 0x04
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
    uint32_t bar0_addr;
    uint32_t bar0_len;
    uint8_t irq;
};

void pci_enable_bus_master(struct pci_device *dev);
error_t pci_find_device(uint16_t vendor, uint16_t device, int *bus, int *slot);
void pci_fill_pci_device_struct(struct pci_device *dev, int bus, int slot);
void pci_init(void);

#endif
