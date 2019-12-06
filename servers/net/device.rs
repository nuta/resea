use crate::ip::{IpAddr, NetworkProtocol};
use crate::ipv4::Ipv4Addr;
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use resea::collections::VecDeque;

#[repr(transparent)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct MacAddr([u8; 6]);

impl MacAddr {
    pub const BROADCAST: MacAddr = MacAddr::new([0xff, 0xff, 0xff, 0xff, 0xff, 0xff]);

    pub const fn new(addr: [u8; 6]) -> MacAddr {
        MacAddr(addr)
    }

    pub fn as_slice(&self) -> &[u8; 6] {
        &self.0
    }
}

pub trait Device {
    fn mac_addr(&self) -> MacAddr;
    fn set_ipv4_addr(&mut self, addr: Ipv4Addr);
    fn enqueue(&mut self, tx_queue: &mut VecDeque<Mbuf>, dst: IpAddr, pkt: Mbuf);
    fn receive(
        &mut self,
        tx_queue: &mut VecDeque<Mbuf>,
        pkt: &mut Packet,
    ) -> Option<NetworkProtocol>;
}
