use crate::pci::Pci;
use resea::channel::Channel;
use resea::collections::{Vec, VecDeque};
use resea::message::Page;
use resea::std::mem::size_of;
use resea::std::sync::atomic::{fence, Ordering};
use resea::utils::align_up;
use resea::PAGE_SIZE;

/// Controls the device features and states.
const REG_CTRL: u32 = 0x0000;
/// Auto-Speed Detetion Enable.
const CTRL_ASDE: u32 = 1 << 5;
/// Set link up.
const CTRL_SLU: u32 = 1 << 6;
/// Device Reset.
const CTRL_RST: u32 = 1 << 26;

/// Interrupt Mask Set.
const REG_IMS: u32 = 0x00d0;
const IMS_RXT0: u32 = 1 << 7;

/// Interrupt Cause Read.
const REG_ICR: u32 = 0x00c0;
/// Receiver Timer Interrupt.
const ICR_RXT0: u32 = 1 << 7;

/// Multicast Table Array.
const REG_MTA_BASE: u32 = 0x5200;
/// The lower bits of the Ethernet address.
const REG_RECEIVE_ADDR_LOW: u32 = 0x5400;
/// The higher bits of the Ethernet address and some extra bits.
const REG_RECEIVE_ADDR_HIGH: u32 = 0x5404;

/// Receive Control.
const REG_RCTL: u32 = 0x0100;
/// Receiver Enable.
const RCTL_EN: u32 = 1 << 1;
/// Strip Ethernet CRC from receiving packet.
const RCTL_SECRC: u32 = 1 << 26;
/// Receive Buffer Size: 2048 bytes (assuming RCTL.BSEX == 0).
const RCTL_BSIZE: u32 = 0 << 16;
/// Broadcast Accept Mode.
const RCTL_BAM: u32 = 1 << 15;

/// Receive Descriptor Base Low.
const REG_RDBAL: u32 = 0x2800;
/// Receive Descriptor Base High.
const REG_RDBAH: u32 = 0x2804;
/// Length of Receive Descriptors.
const REG_RDLEN: u32 = 0x2808;
/// Receive Descriptor Head.
const REG_RDH: u32 = 0x2810;
/// Receive Descriptor Tail.
const REG_RDT: u32 = 0x2818;

/// Transmit Control.
const REG_TCTL: u32 = 0x0400;
/// Receiver Enable.
const TCTL_EN: u32 = 1 << 1;
/// Pad Short Packets.
const TCTL_PSP: u32 = 1 << 3;

/// Transmit Descriptor Base Low.
const REG_TDBAL: u32 = 0x3800;
/// Transmit Descriptor Base High.
const REG_TDBAH: u32 = 0x3804;
/// Length of Transmit Descriptors.
const REG_TDLEN: u32 = 0x3808;
/// Transmit Descriptor Head.
const REG_TDH: u32 = 0x3810;
/// Transmit Descriptor Tail.
const REG_TDT: u32 = 0x3818;

/// The size of buffer to store received/transmtting packets.
const BUFFER_SIZE: usize = 2048;
/// Number of receive descriptors.
const NUM_RX_DESCS: usize = 32;
/// Number of receive descriptors.
const NUM_TX_DESCS: usize = 32;

#[repr(C, packed)]
struct RxDesc {
    paddr: u64,
    len: u16,
    checksum: u16,
    status: u8,
    errors: u8,
    special: u16,
}

/// Descriptor Done.
const RX_DESC_DD: u8 = 1 << 0;
/// End Of Packet.
const RX_DESC_EOP: u8 = 1 << 1;

#[repr(C, packed)]
struct TxDesc {
    paddr: u64,
    len: u16,
    cso: u8,
    cmd: u8,
    status: u8,
    css: u8,
    special: u16,
}

/// Insert FCS.
const TX_DESC_IFCS: u8 = 1 << 1;
/// End Of Packet.
const TX_DESC_EOP: u8 = 1 << 0;

pub struct Device {
    regs_page: Page,
    tx_desc_page: Page,
    rx_desc_page: Page,
    tx_data_page: Page,
    rx_data_page: Page,
    rx_desc_paddr: usize,
    tx_desc_paddr: usize,
    rx_data_paddr: usize,
    tx_data_paddr: usize,
    tx_current: u32,
    rx_current: u32,
    rx_queue: VecDeque<Vec<u8>>,
}

impl Device {
    pub fn new(
        server_ch: &Channel,
        pci: &Pci,
        kernel_server: &'static Channel,
        memmgr: &'static Channel,
    ) -> Device {
        let pci_device = pci
            .find_device(0x8086, 0x100e)
            .expect("failed to locate an e1000 device in PCI");
        pci.enable_bus_master(&pci_device);

        use resea::idl::kernel::Client as KernelClient;
        let irq_ch = Channel::create().unwrap();
        irq_ch.transfer_to(server_ch).unwrap();
        kernel_server
            .listen_irq(irq_ch, pci_device.interrupt_line)
            .expect("failed to register the keyboard IRQ handler");

        // Map memory-mapped registers.
        use resea::idl::memmgr::Client;
        let regs_page = memmgr.map_phy_pages(pci_device.bar0 as usize, 8).unwrap();

        // Allocate pages for RX/TX descriptors.
        let rx_desc_len = align_up(size_of::<RxDesc>() * NUM_RX_DESCS, PAGE_SIZE);
        let (rx_desc_paddr, rx_desc_page) =
            memmgr.alloc_phy_pages(rx_desc_len / PAGE_SIZE).unwrap();
        let tx_buffer_len = align_up(size_of::<TxDesc>() * NUM_TX_DESCS, PAGE_SIZE);
        let (tx_desc_paddr, tx_desc_page) =
            memmgr.alloc_phy_pages(tx_buffer_len / PAGE_SIZE).unwrap();

        // Allocate pages for RX/TX packets.
        let rx_desc_len = align_up(BUFFER_SIZE * NUM_RX_DESCS, PAGE_SIZE);
        let (rx_data_paddr, rx_data_page) =
            memmgr.alloc_phy_pages(rx_desc_len / PAGE_SIZE).unwrap();
        let tx_buffer_len = align_up(BUFFER_SIZE * NUM_TX_DESCS, PAGE_SIZE);
        let (tx_data_paddr, tx_data_page) =
            memmgr.alloc_phy_pages(tx_buffer_len / PAGE_SIZE).unwrap();

        Device {
            regs_page,
            tx_desc_page,
            rx_desc_page,
            tx_data_page,
            rx_data_page,
            rx_desc_paddr,
            tx_desc_paddr,
            rx_data_paddr,
            tx_data_paddr,
            tx_current: 0,
            rx_current: 0,
            rx_queue: VecDeque::new(),
        }
    }

    pub fn init(&mut self) {
        // Reset the device.
        self.write_reg32(REG_CTRL, self.read_reg32(REG_CTRL) | CTRL_RST);

        // Wait until the device gets reset.
        while self.read_reg32(REG_CTRL) & CTRL_RST != 0 {}

        // Link up!
        self.write_reg32(REG_CTRL, self.read_reg32(REG_CTRL) | CTRL_SLU | CTRL_ASDE);

        // Fill Multicast Table Array with zeros.
        for i in 0..0x80 {
            self.write_reg32(REG_MTA_BASE + i * 4, 0);
        }

        // Initialize RX queue.
        let rx_data_paddr = self.rx_data_paddr;
        for i in 0..NUM_RX_DESCS {
            let desc = self.rx_desc(i as u32);
            desc.paddr = (rx_data_paddr + i * BUFFER_SIZE) as u64;
            desc.status = 0;
        }

        self.write_reg32(REG_RDBAL, (self.rx_desc_paddr & 0xffff_ffff) as u32);
        self.write_reg32(REG_RDBAH, (self.rx_desc_paddr >> 32) as u32);
        self.write_reg32(REG_RDLEN, (NUM_RX_DESCS * size_of::<RxDesc>()) as u32);
        self.write_reg32(REG_RDH, 0);
        self.write_reg32(REG_RDT, NUM_RX_DESCS as u32);
        self.write_reg32(REG_RCTL, RCTL_EN | RCTL_SECRC | RCTL_BSIZE | RCTL_BAM);

        // Initialize TX queue.
        let tx_data_paddr = self.tx_data_paddr;
        for i in 0..NUM_TX_DESCS {
            let desc = self.tx_desc(i as u32);
            desc.paddr = (tx_data_paddr + i * BUFFER_SIZE) as u64;
        }

        self.write_reg32(REG_TDBAL, (self.tx_desc_paddr & 0xffff_ffff) as u32);
        self.write_reg32(REG_TDBAH, (self.tx_desc_paddr >> 32) as u32);
        self.write_reg32(REG_TDLEN, (NUM_TX_DESCS * size_of::<TxDesc>()) as u32);
        self.write_reg32(REG_TDH, 0);
        self.write_reg32(REG_TDT, 0);
        self.write_reg32(REG_TCTL, TCTL_EN | TCTL_PSP);

        // Enable interrupts.
        self.write_reg32(REG_IMS, IMS_RXT0);
    }

    pub fn send_ethernet_packet(&mut self, pkt: &[u8]) {
        // Copy the packet into the TX buffer.
        let buffer = self.tx_buffer(self.tx_current);
        buffer[0..(pkt.len())].copy_from_slice(pkt);

        // Set descriptor fields. The buffer address field is already filled
        // during initialization.
        let desc = self.tx_desc(self.tx_current);
        desc.cmd = TX_DESC_IFCS | TX_DESC_EOP;
        desc.len = pkt.len() as u16;
        desc.cso = 0;
        desc.status = 0;
        desc.css = 0;
        desc.special = 0;

        // Notify the device.
        self.tx_current = (self.tx_current + 1) % (NUM_TX_DESCS as u32);
        self.write_reg32(REG_TDT, self.tx_current);

        // TODO: Wait until trasmission finish.
    }

    pub fn rx_queue(&mut self) -> &mut VecDeque<Vec<u8>> {
        &mut self.rx_queue
    }

    pub fn handle_interrupt(&mut self) {
        let cause = self.read_reg32(REG_ICR);
        if cause & ICR_RXT0 != 0 {
            self.received();
        }
    }

    fn received(&mut self) {
        loop {
            let index = self.rx_current;
            let desc = self.rx_desc(index);

            // We don't support a large packet which spans multiple descriptors.
            let bits = RX_DESC_DD | RX_DESC_EOP;
            if desc.status & bits != bits {
                break;
            }

            // Copy the received packet into the RX queue.
            let len = desc.len as usize;
            warn!("received {} bytes", len);
            let mut pkt = Vec::with_capacity(len);
            pkt.extend_from_slice(&self.rx_buffer(index)[0..len]);
            self.rx_queue.push_back(pkt);

            // Tell the device that we've processed a received packet.
            self.rx_desc(index).status = 0;
            self.rx_current = (index + 1) % (NUM_RX_DESCS as u32);
            self.write_reg32(REG_RDT, index);
        }
    }

    pub fn mac_addr(&self) -> [u8; 6] {
        let mut mac_addr = [0; 6];

        for i in 0..4 {
            mac_addr[i] = self.read_reg8(REG_RECEIVE_ADDR_LOW + i as u32);
        }

        for i in 0..2 {
            mac_addr[4 + i] = self.read_reg8(REG_RECEIVE_ADDR_HIGH + i as u32);
        }

        mac_addr
    }

    fn read_reg8(&self, offset: u32) -> u8 {
        fence(Ordering::Acquire);
        let aligned_offset = offset & 0xffff_fffc;
        let value = self.read_reg32(aligned_offset);
        ((value >> ((offset & 0x03) * 8)) & 0xff) as u8
    }

    fn read_reg32(&self, offset: u32) -> u32 {
        fence(Ordering::Acquire);
        unsafe {
            let ptr = self.regs_page.as_bytes().as_ptr().offset(offset as isize) as *const u32;
            core::ptr::read_volatile(ptr)
        }
    }

    fn write_reg32(&self, offset: u32, value: u32) {
        unsafe {
            let ptr = self.regs_page.as_bytes().as_ptr().offset(offset as isize) as *mut u32;
            core::ptr::write_volatile(ptr, value);
        }

        fence(Ordering::Acquire);
    }

    fn rx_desc(&mut self, index: u32) -> &mut RxDesc {
        &mut self.rx_desc_page.as_slice_mut::<RxDesc>()[index as usize]
    }

    fn tx_desc(&mut self, index: u32) -> &mut TxDesc {
        &mut self.tx_desc_page.as_slice_mut::<TxDesc>()[index as usize]
    }

    fn rx_buffer(&mut self, index: u32) -> &[u8] {
        let start = (index as usize) * BUFFER_SIZE;
        let end = (index as usize + 1) * BUFFER_SIZE;
        &self.rx_data_page.as_bytes()[start..end]
    }

    fn tx_buffer(&mut self, index: u32) -> &mut [u8] {
        let start = (index as usize) * BUFFER_SIZE;
        let end = (index as usize + 1) * BUFFER_SIZE;
        &mut self.tx_data_page.as_bytes_mut()[start..end]
    }
}
