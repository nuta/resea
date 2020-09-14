#include <driver/io.h>
#include <resea/printf.h>
#include "pci.h"

static io_t io;

static uint32_t read32(uint8_t bus, uint8_t slot, uint16_t offset) {
    ASSERT(IS_ALIGNED(offset, 4));
    uint32_t addr = (1UL << 31) | (bus << 16) | (slot << 11) | offset;
    io_write32(io, PCI_IOPORT_ADDR, addr);
    return io_read32(io, PCI_IOPORT_DATA);
}

static uint8_t read8(uint8_t bus, uint8_t slot, uint16_t offset) {
    uint32_t value = read32(bus, slot, offset & 0xfffc);
    return (value >> ((offset & 0x03) * 8)) & 0xff;
}

static uint16_t read16(uint8_t bus, uint8_t slot, uint16_t offset) {
    uint32_t value = read32(bus, slot, offset & 0xfffc);
    return (value >> ((offset & 0x03) * 8)) & 0xffff;
}

static void write32(uint8_t bus, uint8_t slot, uint16_t offset,
                    uint32_t value) {
    ASSERT(IS_ALIGNED(offset, 4));
    uint32_t addr = (1UL << 31) | (bus << 16) | (slot << 11) | offset;
    io_write32(io, PCI_IOPORT_ADDR, addr);
    io_write32(io, PCI_IOPORT_DATA, value);
}

void pci_enable_bus_master(struct pci_device *dev) {
    uint32_t value = read32(dev->bus, dev->slot, PCI_CONFIG_COMMAND) | (1 << 2);
    write32(dev->bus, dev->slot, PCI_CONFIG_COMMAND, value);
}

error_t pci_find_device(uint16_t vendor, uint16_t device, int *bus_out, int *slot_out) {
    for (int bus = 0; bus <= 255; bus++) {
        for (int slot = 0; slot < 32; slot++) {
            uint16_t vendor2 = read16(bus, slot, PCI_CONFIG_VENDOR_ID);
            uint16_t device2 = read16(bus, slot, PCI_CONFIG_DEVICE_ID);
            if (vendor2 == 0xffff || (vendor != PCI_ANY && vendor != vendor2)) {
                continue;
            }

            if (device2 != PCI_ANY && device2 != device) {
                continue;
            }

            *bus_out = bus;
            *slot_out = slot;
            return OK;
        }
    }

    return ERR_NOT_FOUND;
}

uint32_t pci_read_config(struct pci_device *dev, unsigned offset, unsigned size) {
    switch (size) {
        case 1:
            return read8(dev->bus, dev->slot, offset);
        case 2:
            return read16(dev->bus, dev->slot, offset);
        case 4:
            return read32(dev->bus, dev->slot, offset);
        default:
            return 0;
    }
}

void pci_write_config(struct pci_device *dev, unsigned offset, unsigned size,
                      uint32_t value) {
    switch (size) {
        case 1:
//            write8(dev->bus, dev->slot, offset, value);
            NYI();
            break;
        case 2:
//            write16(dev->bus, dev->slot, offset, value);
            NYI();
            break;
        case 4:
            write32(dev->bus, dev->slot, offset, value);
            break;
    }
}

void pci_fill_pci_device_struct(struct pci_device *dev, int bus, int slot) {
    uint32_t bar0_addr = read32(bus, slot, PCI_CONFIG_BAR0);

    // Determine the size of the space.
    // https://wiki.osdev.org/PCI#Base_Address_Registers
    write32(bus, slot, PCI_CONFIG_BAR0, 0xffffffff);
    uint32_t bar0_len = ~(read32(bus, slot, PCI_CONFIG_BAR0) & (~0xf)) + 1;

    // Restore the original value.
    write32(bus, slot, PCI_CONFIG_BAR0, bar0_addr);

    dev->bus = bus;
    dev->slot = slot;
    dev->vendor = read16(bus, slot, PCI_CONFIG_VENDOR_ID);
    dev->device = read16(bus, slot, PCI_CONFIG_DEVICE_ID);
    dev->bar0_addr = bar0_addr;
    dev->bar0_len = bar0_len;
    dev->irq = read8(bus, slot, PCI_CONFIG_INTR_LINE);
}

void pci_init(void) {
    io = io_alloc_port(PCI_IOPORT_BASE, 8, IO_ALLOC_NORMAL);
}
