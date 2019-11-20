use crate::ip::{IpAddr, NetworkProtocol};
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use crate::Result;
use resea::collections::VecDeque;

pub trait Device {
    fn enqueue(&mut self, tx_queue: &mut VecDeque<Mbuf>, dst: IpAddr, pkt: Mbuf) -> Result<()>;
    fn receive(
        &mut self,
        tx_queue: &mut VecDeque<Mbuf>,
        pkt: &mut Packet,
    ) -> Option<NetworkProtocol>;
}
