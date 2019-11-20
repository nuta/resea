use resea::channel::Channel;

const PCI_IOPORT_ADDR: u64 = 0x0cf8;
const PCI_IOPORT_DATA: u64 = 0x0cfc;
const PCI_ANY: u16 = 0;

const PCI_CONFIG_VENDOR_ID: u16 = 0x00;
const PCI_CONFIG_DEVICE_ID: u16 = 0x02;
const PCI_CONFIG_BAR0: u16 = 0x10;
const PCI_CONFIG_INTR_LINE: u16 = 0x3c;

pub struct PciDevice {
    pub bus: u8,
    pub slot: u8,
    pub vendor: u16,
    pub device: u16,
    pub bar0: u32,
    pub interrupt_line: u8,
}

/// TODO: Manage PCI devices in an external server.
pub struct Pci {
    kernel_server: &'static Channel,
}

impl Pci {
    pub fn new(kernel_server: &'static Channel) -> Pci {
        Pci { kernel_server }
    }

    pub fn find_device(&self, vendor: u16, device: u16) -> Option<PciDevice> {
        for bus in 0..=255 {
            for slot in 0..32 {
                let vendor_id = self.read_config16(bus, slot, PCI_CONFIG_VENDOR_ID);
                let device_id = self.read_config16(bus, slot, PCI_CONFIG_DEVICE_ID);
                if vendor_id == 0xffff || (vendor != PCI_ANY && vendor != vendor_id) {
                    continue;
                }

                if device_id != PCI_ANY && device_id != device {
                    continue;
                }

                return Some(PciDevice {
                    bus,
                    slot,
                    vendor: vendor_id,
                    device: device_id,
                    interrupt_line: self.read_config8(bus, slot, PCI_CONFIG_INTR_LINE),
                    bar0: self.read_config32(bus, slot, PCI_CONFIG_BAR0),
                });
            }
        }

        None
    }

    pub fn enable_bus_master(&self, device: &PciDevice) {
        let value = self.read_config32(device.bus, device.slot, 0x04) | (1 << 2);
        self.write_config32(device.bus, device.slot, 0x04, value as u32);
    }

    fn read_config8(&self, bus: u8, slot: u8, offset: u16) -> u8 {
        let value = self.read_config32(bus, slot, offset & 0xfffc);
        ((value >> ((offset & 0x03) * 8)) & 0xff) as u8
    }

    fn read_config16(&self, bus: u8, slot: u8, offset: u16) -> u16 {
        let value = self.read_config32(bus, slot, offset & 0xfffc);
        ((value >> ((offset & 0x03) * 8)) & 0xffff) as u16
    }

    fn read_config32(&self, bus: u8, slot: u8, offset: u16) -> u32 {
        // Make sure the offset is aligned.
        assert!((offset & 0x3) == 0);

        let addr = (1 << 31) | ((bus as u32) << 16) | ((slot as u32) << 11) | offset as u32;

        use resea::idl::kernel::Client;
        self.kernel_server
            .write_ioport(PCI_IOPORT_ADDR, 4, addr as u64)
            .unwrap();
        self.kernel_server.read_ioport(PCI_IOPORT_DATA, 4).unwrap() as u32
    }

    fn write_config32(&self, bus: u8, slot: u8, offset: u16, value: u32) {
        // Make sure the offset is aligned.
        assert!((offset & 0x3) == 0);

        let addr = (1 << 31) | ((bus as u32) << 16) | ((slot as u32) << 11) | offset as u32;

        use resea::idl::kernel::Client;
        self.kernel_server
            .write_ioport(PCI_IOPORT_ADDR, 4, addr as u64)
            .unwrap();
        self.kernel_server
            .write_ioport(PCI_IOPORT_DATA, 4, value as u64)
            .unwrap();
    }
}
