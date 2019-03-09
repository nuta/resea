use resea_driver::{IoSpace, IoSpaceType};
use core::mem::size_of;

#[derive(Debug, Copy, Clone)]
#[repr(C, packed)]
struct PciConfig {
    vendor_id: u16,
    device_id: u16,
    command: u16,
    status: u16,
    revision_id: u8,
    prog_if: u8,
    subclass: u8,
    class: u8,
    cache_line_size: u8,
    latency_timer: u8,
    header_type: u8,
    bist: u8,
    bar0: u32,
    bar1: u32,
    bar2: u32,
    bar3: u32,
    bar4: u32,
    bar5: u32,
    cardbus: u32,
    subsys_vendor_id: u16,
    subsys_id: u16,
    rom_base: u32,
    capabilities: u8,
    _reserved0: [u8; 7],
    interrupt_line: u8,
    interrupt_pin: u8,
    min_grant: u8,
    max_latency: u8,
}

const PCI_IO_ADDR: usize = 0x00;
const PCI_IO_DATA: usize = 0x04;

fn read_pci_config(io: &mut IoSpace, bus: usize, dev: usize) -> PciConfig {
    unsafe {
        let mut config: PciConfig = core::mem::uninitialized();
        let config_ptr = core::mem::transmute::<*mut PciConfig, *mut u32>(&mut config);

        for i in 0..(size_of::<PciConfig>() / size_of::<u32>()) {
            let offset = i * size_of::<u32>();
            let addr = (1 << 31) | (bus << 16) | (dev << 11) | offset;
            io.write32(PCI_IO_ADDR, addr as u32);
            let data = io.read32(PCI_IO_DATA);
            config_ptr.add(i).write(data);
        }

        config
    }
}

pub fn scan_pci_bus() {
    let mut io = IoSpace::new(IoSpaceType::IoMappedIo, 0xcf8, 8);

    for bus in 0..=255 {
        for dev in 0..32 {
            let config = read_pci_config(&mut io, bus, dev);
            if config.vendor_id != 0xffff {
                println!("found a PCI device: {:x?}", config);
            }
        }
    }
}
