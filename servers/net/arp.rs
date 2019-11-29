use crate::device::MacAddr;
use crate::endian::NetEndian;
use crate::ipv4::Ipv4Addr;
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use resea::collections::HashMap;
use resea::vec::Vec;

pub enum ArpEntry {
    /// The MAC address of the IP address.
    /// TODO: Support expiration.
    Resolved(MacAddr),
    /// Sent a ARP request, but we have not yet received the reply. This element
    /// contains pending packets (w/o ethernet header) that we'll send to the IP
    /// address once we got a reply.
    UnResolved(Vec<(u16, Mbuf)>),
}

pub struct ArpTable {
    table: HashMap<Ipv4Addr, ArpEntry>,
    mac_addr: MacAddr,
    ipv4_addr: Ipv4Addr,
}

impl ArpTable {
    pub fn new(mac_addr: MacAddr) -> ArpTable {
        ArpTable {
            table: HashMap::new(),
            mac_addr,
            ipv4_addr: Ipv4Addr::UNSPECIFIED,
        }
    }

    pub fn set_ipv4_addr(&mut self, addr: Ipv4Addr) {
        self.ipv4_addr = addr;
    }

    pub fn resolve(&self, addr: Ipv4Addr) -> Option<MacAddr> {
        if addr == Ipv4Addr::BROADCAST {
            return Some(MacAddr::BROADCAST);
        }

        match self.table.get(&addr) {
            Some(ArpEntry::Resolved(mac_addr)) => Some(*mac_addr),
            _ => None,
        }
    }

    /// Enqueue a packet and build an ARP request.
    pub fn enqueue(&mut self, addr: Ipv4Addr, ether_type: u16, pkt: Mbuf) -> Mbuf {
        match self.table.get_mut(&addr) {
            Some(ArpEntry::Resolved(_)) => unreachable!(),
            Some(ArpEntry::UnResolved(pending)) => pending.push((ether_type, pkt)),
            None => {
                self.table
                    .insert(addr, ArpEntry::UnResolved(vec![(ether_type, pkt)]));
            }
        }

        let mut mbuf = Mbuf::new();
        build(
            &mut mbuf,
            OPCODE_REQUEST,
            MacAddr::BROADCAST,
            addr,
            self.mac_addr,
            self.ipv4_addr,
        );
        mbuf
    }

    pub fn process_arp_packet(&mut self, pkt: &mut Packet) -> Option<(MacAddr, Vec<(u16, Mbuf)>)> {
        let packet = match pkt.consume::<ArpPacket>() {
            Some(packet) => packet,
            None => return None,
        };

        let dst = Ipv4Addr::from_u32(packet.target_ipv4.into());
        let src = Ipv4Addr::from_u32(packet.sender_ipv4.into());

        match packet.opcode.into() {
            OPCODE_REQUEST if self.ipv4_addr == dst => {
                trace!("received arp request");
                let mut mbuf = Mbuf::new();
                mbuf.prepend(&ArpPacket {
                    hw_type: 1.into(),         // Ethernet
                    proto_type: 0x0800.into(), // IPv4
                    hw_size: 6,
                    proto_size: 4,
                    opcode: OPCODE_REPLY.into(),
                    sender: self.mac_addr,
                    sender_ipv4: self.ipv4_addr.as_u32().into(),
                    target: packet.sender,
                    target_ipv4: packet.sender_ipv4,
                });

                Some((packet.sender, vec![(0x0806 /* ARP */, mbuf)]))
            }
            OPCODE_REPLY => {
                trace!("received arp reply from: {}", src);
                if let Some(ArpEntry::UnResolved(pending)) = self.table.remove(&src) {
                    trace!("received arp reply");
                    self.table.insert(src, ArpEntry::Resolved(packet.sender));
                    Some((packet.sender, pending))
                } else {
                    None
                }
            }
            _ => None,
        }
    }
}

#[repr(C, packed)]
struct ArpPacket {
    hw_type: NetEndian<u16>,
    proto_type: NetEndian<u16>,
    hw_size: u8,
    proto_size: u8,
    opcode: NetEndian<u16>,
    sender: MacAddr,
    sender_ipv4: NetEndian<u32>,
    target: MacAddr,
    target_ipv4: NetEndian<u32>,
}

const OPCODE_REQUEST: u16 = 1;
const OPCODE_REPLY: u16 = 2;

fn build(
    pkt: &mut Mbuf,
    opcode: u16,
    dst: MacAddr,
    dst_addr: Ipv4Addr,
    src: MacAddr,
    src_addr: Ipv4Addr,
) {
    pkt.prepend(&ArpPacket {
        hw_type: 1.into(),         // Ethernet
        proto_type: 0x0800.into(), // IPv4
        hw_size: 6,
        proto_size: 4,
        opcode: opcode.into(),
        sender: src,
        sender_ipv4: src_addr.as_u32().into(),
        target: dst,
        target_ipv4: dst_addr.as_u32().into(),
    });
}
