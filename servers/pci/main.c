#include <resea.h>
#include <idl_stubs.h>
#include <server.h>
#include <driver/io.h>
#include <driver/pci.h>
#include "pci.h"

// Channels connected by memmgr.
static cid_t memmgr_ch = 1;
static cid_t kernel_ch = 2;
// The channel at which we receive messages.
static cid_t server_ch;

static io_handle_t io_handle;

static uint32_t read_config32(uint8_t bus, uint8_t dev, uint16_t offset) {
    assert((offset & 0x3) == 0); // Make sure the offset is aligned.

    uint32_t addr = (1UL << 31) | ((uint32_t) bus << 16)
                  | ((uint32_t)  dev << 11) | offset;
    io_write32(&io_handle, IOPORT_OFFSET_ADDR, addr);
    return io_read32(&io_handle, IOPORT_OFFSET_DATA);
}

static uint16_t read_config16(uint8_t bus, uint8_t dev, uint16_t offset) {
    uint32_t value = read_config32(bus, dev, offset & 0xfc);
    return (value >> ((offset & 0x3) * 8)) & 0xffff;
}

static void write_config32(uint8_t bus, uint8_t dev, uint16_t offset,
                           uint32_t data) {
    assert((offset & 0x3) == 0); // Make sure the offset is aligned.

    uint32_t addr = (1UL << 31) | ((uint32_t) bus << 16)
                  | ((uint32_t)  dev << 11) | offset;
    io_write32(&io_handle, IOPORT_OFFSET_ADDR, addr);
    return io_write32(&io_handle, IOPORT_OFFSET_DATA, data);
}

static void write_config8(uint8_t bus, uint8_t dev, uint16_t offset,
                          uint8_t data) {
    uint16_t aligned_offset = offset & 0xfc;
    uint32_t value = read_config32(bus, dev, aligned_offset);
    uint8_t byte_off = (offset & 0x3) * 8;
    uint32_t new_value = (data << byte_off) | (value & ~(0xffUL << byte_off));
    return write_config32(bus, dev, aligned_offset, new_value);
}

static error_t look_for_device(uint16_t *vendor, uint16_t *device,
                               uint16_t *subvendor, uint16_t *subdevice,
                               uint8_t *bus, uint8_t *slot) {
    for (int i = 0; i < 256; i++) {
        for (uint8_t j = 0; j < 32; j++) {
            uint16_t vendor_id = read_config16(i, j, PCI_CONFIG_VENDOR_ID);
            if (vendor_id == 0xffff) {
                continue;
            }

            if (*vendor != PCI_ANY && vendor_id != *vendor) {
                continue;
            }

            uint16_t device_id = read_config16(i, j, PCI_CONFIG_DEVICE_ID);
            if (*device != PCI_ANY && device_id != *device) {
                continue;
            }

            // Found the device!
            *vendor    = vendor_id;
            *device    = device_id;
            *subvendor = read_config16(i, j, PCI_CONFIG_SUBVENDOR_ID);
            *subdevice = read_config16(i, j, PCI_CONFIG_SUBDEVICE_ID);
            *bus       = i;
            *slot      = j;
            return OK;
        }
    }

    return ERR_NOT_FOUND;
}

static error_t handle_pci_wait_for_device(struct message *m) {
    uint16_t vendor = m->payloads.pci.wait_for_device.vendor;
    uint16_t device = m->payloads.pci.wait_for_device.device;

    uint16_t subvendor, subdevice;
    uint8_t bus, slot;
    error_t err = look_for_device(&vendor, &device, &subvendor, &subdevice,
                                  &bus, &slot);
    if (err == ERR_NOT_FOUND) {
        WARN("PCI device %x:%x does not exist returning ERR_NOT_FOUND.",
             vendor, device);
        // TODO: Support PCI hot plug.
        return ERR_NOT_FOUND;
    }

    m->header = PCI_WAIT_FOR_DEVICE_REPLY_MSG;
    m->payloads.pci.wait_for_device_reply.bus = bus;
    m->payloads.pci.wait_for_device_reply.slot = slot;
    return OK;
}

static error_t handle_pci_read_config16(struct message *m) {
    uint8_t bus     = m->payloads.pci.read_config16.bus;
    uint8_t slot    = m->payloads.pci.read_config16.slot;
    uint16_t offset = m->payloads.pci.read_config16.offset;

    uint16_t data = read_config16(bus, slot, offset);

    m->header = PCI_READ_CONFIG16_REPLY_MSG;
    m->payloads.pci.read_config16_reply.data = data;
    return OK;
}

static error_t handle_pci_read_config32(struct message *m) {
    uint16_t bus    = m->payloads.pci.read_config32.bus;
    uint16_t slot   = m->payloads.pci.read_config32.slot;
    uint16_t offset = m->payloads.pci.read_config32.offset;

    uint32_t data = read_config32(bus, slot, offset);

    m->header = PCI_READ_CONFIG32_REPLY_MSG;
    m->payloads.pci.read_config32_reply.data = data;
    return OK;
}

static error_t handle_pci_write_config8(struct message *m) {
    uint8_t bus     = m->payloads.pci.write_config8.bus;
    uint8_t slot    = m->payloads.pci.write_config8.slot;
    uint16_t offset = m->payloads.pci.write_config8.offset;
    uint8_t data    = m->payloads.pci.write_config8.data;

    write_config8(bus, slot, offset, data);

    m->header = PCI_WRITE_CONFIG8_REPLY_MSG;
    return OK;
}

static error_t handle_pci_write_config32(struct message *m) {
    uint8_t bus     = m->payloads.pci.write_config32.bus;
    uint8_t slot    = m->payloads.pci.write_config32.slot;
    uint16_t offset = m->payloads.pci.write_config32.offset;
    uint32_t data   = m->payloads.pci.write_config32.data;

    write_config32(bus, slot, offset, data);

    m->header = PCI_WRITE_CONFIG32_REPLY_MSG;
    return OK;
}

static error_t handle_server_connect(struct message *m) {
    cid_t new_ch;
    TRY(open(&new_ch));
    transfer(new_ch, server_ch);

    m->header = SERVER_CONNECT_REPLY_MSG;
    m->payloads.server.connect_reply.ch = new_ch;
    return OK;
}

static error_t process_message(struct message *m) {
    switch (m->header) {
    case SERVER_CONNECT_MSG:      return handle_server_connect(m);
    case PCI_WAIT_FOR_DEVICE_MSG: return handle_pci_wait_for_device(m);
    case PCI_READ_CONFIG16_MSG:   return handle_pci_read_config16(m);
    case PCI_READ_CONFIG32_MSG:   return handle_pci_read_config32(m);
    case PCI_WRITE_CONFIG8_MSG:   return handle_pci_write_config8(m);
    case PCI_WRITE_CONFIG32_MSG:  return handle_pci_write_config32(m);
    }
    return ERR_UNEXPECTED_MESSAGE;
}

void main(void) {
    INFO("starting...");

    TRY_OR_PANIC(open(&server_ch));
    TRY_OR_PANIC(io_open(&io_handle, kernel_ch, IO_SPACE_IOPORT,
                         PCI_IOPORT_ADDR, PCI_IOPORT_SIZE));
    TRY_OR_PANIC(server_register(memmgr_ch, server_ch,
                                 PCI_INTERFACE));

    server_mainloop(server_ch, process_message);
}
