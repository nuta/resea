use crate::Result;
use resea::collections::Vec;
use resea::collections::VecDeque;
use crate::device::Device;
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use crate::ip::{IpAddr, NetworkProtocol};
use crate::ipv4::Ipv4Addr;
use crate::arp::ArpTable;
use crate::endian::NetEndian;

#[repr(transparent)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct MacAddr([u8; 6]);

impl MacAddr {
    pub const BROADCAST: MacAddr =
        MacAddr::new([0xff, 0xff, 0xff, 0xff, 0xff, 0xff]);

    pub const fn new(addr: [u8; 6]) -> MacAddr {
        MacAddr(addr)
    }
}

pub struct EthernetDevice {
    arp_table: ArpTable,
    mac_addr: MacAddr,
}

impl EthernetDevice {
    pub fn new(mac_addr: MacAddr) -> EthernetDevice {
        EthernetDevice {
            arp_table: ArpTable::new(mac_addr),
            mac_addr,
        }
    }

    pub fn set_ipv4_addr(&mut self, addr: Ipv4Addr) {
        self.arp_table.set_ipv4_addr(addr);
    }

    fn receive_arp_packet(&mut self, pkt: &mut Packet) -> Option<(MacAddr, Vec<(u16, Mbuf)>)> {
        self.arp_table.process_arp_packet(pkt)
    }

    fn do_enqueue(&mut self, queue: &mut VecDeque<Mbuf>, dst_macaddr: MacAddr, ether_type: u16, mut pkt: Mbuf) {
        build(&mut pkt, dst_macaddr, self.mac_addr, ether_type);
        queue.push_back(pkt);
    }
}

impl Device for EthernetDevice {
    fn enqueue(&mut self, tx_queue: &mut VecDeque<Mbuf>, dst: IpAddr, mut pkt: Mbuf) -> Result<()> {
        let (ether_type, dst_macaddr) = match dst {
            IpAddr::Ipv4(addr) => {
                match self.arp_table.resolve(addr) {
                    Some(mac_addr) => (ETHERTYPE_IPV4, mac_addr),
                    None => {
                        // We don't know the MAC address. Enqueue the packet and
                        // send a ARP request.
                        let mut arp_req = self.arp_table.enqueue(addr, ETHERTYPE_IPV4, pkt);
                        self.do_enqueue(tx_queue, MacAddr::BROADCAST, ETHERTYPE_ARP, arp_req);
                        return Ok(());
                    }
                }
            }
        };

        self.do_enqueue(tx_queue, dst_macaddr, ether_type, pkt);
        Ok(())
    }

    fn receive(&mut self, tx_queue: &mut VecDeque<Mbuf>, pkt: &mut Packet) -> Option<NetworkProtocol> {
        let header = match pkt.consume::<EthernetHeader>() {
            Some(header) => header,
            None => return None,
        };
        
        let proto = match header.ether_type.into() {
            ETHERTYPE_IPV4 => {
                NetworkProtocol::Ipv4
            }
            ETHERTYPE_ARP => {
                let (dst_macaddr, pending_packets) =
                    match self.receive_arp_packet(pkt) {
                        Some(values) => values,
                        None => return None,
                    };

                for (ether_type, packet) in pending_packets {
                    self.do_enqueue(tx_queue, dst_macaddr, ether_type, packet);
                }

                return None;
            }
            _ => {
                // Unsupported protocol.
                return None;
            }
        };

        Some(proto)
    }
}

#[repr(C, packed)]
struct EthernetHeader {
    dst: MacAddr,
    src: MacAddr,
    ether_type: NetEndian<u16>,
}

const ETHERTYPE_IPV4: u16 = 0x0800;
const ETHERTYPE_ARP: u16 = 0x0806;

fn build(mbuf: &mut Mbuf, dst: MacAddr, src: MacAddr, ether_type: u16) {
    let header = EthernetHeader {
        dst,
        src,
        ether_type: ether_type.into(),
    };

    mbuf.prepend(&header);
}
