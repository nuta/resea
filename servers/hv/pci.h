#ifndef __PCI_H__
#define __PCI_H__

#include <list.h>
#include <types.h>

struct __packed pci_config {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t rev_id;
    uint8_t prog_if;
    uint8_t subclass;
    uint8_t class;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint32_t cardbus;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base;
    uint8_t cap_pointer;
    uint8_t reserved[7];
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t min_latency;
};

struct pci_device;
struct pci_device_ops {
    uint32_t (*ioport_read)(struct pci_device *device, uint64_t port,
                            size_t size);
    void (*ioport_write)(struct pci_device *device, uint64_t port, size_t size,
                         uint32_t value);
};

struct guest;
struct pci_device {
    list_elem_t next;
    const char *name;
    struct guest *guest;
    struct pci_device_ops *ops;
    struct pci_config *config;
    void *data;
    int slot;
    uint16_t port_base;
    uint16_t port_len;
    uint8_t irq;
};

uint32_t pci_handle_io_read(struct guest *guest, uint64_t port, size_t size);
void pci_handle_io_write(struct guest *guest, uint64_t port, size_t size,
                         uint32_t value);
void pci_add_device(struct guest *guest, const char *name,
                    struct pci_device_ops *ops, struct pci_config *config,
                    void *data, size_t port_len, bool use_irq);
void pci_init(struct guest *guest);

#endif
