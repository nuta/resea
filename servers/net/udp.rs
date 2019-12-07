use crate::device::Device;
use crate::endian::NetEndian;
use crate::ip::IpAddr;
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use crate::tcp::TcpSocket;
use crate::transport::{
    BindTo, Port, Socket, TransportHeader, TransportProtocol, UdpTransportHeader,
};
use crate::Result;
use resea::cell::RefCell;
use resea::collections::VecDeque;
use resea::mem::size_of;
use resea::rc::Rc;
use resea::vec::Vec;

struct TxPacket {
    device: Option<Rc<RefCell<dyn Device>>>,
    dst_addr: IpAddr,
    dst_port: Port,
    payload: Vec<u8>,
}

struct RxPacket {
    src_addr: IpAddr,
    src_port: Port,
    payload: Vec<u8>,
}

pub struct UdpSocket {
    bind_to: BindTo,
    tx: VecDeque<TxPacket>,
    rx: VecDeque<RxPacket>,
}

impl UdpSocket {
    pub fn new(bind_to: BindTo) -> UdpSocket {
        UdpSocket {
            bind_to,
            tx: VecDeque::new(),
            rx: VecDeque::new(),
        }
    }
}

impl Socket for UdpSocket {
    fn build(&mut self) -> Option<(Option<Rc<RefCell<dyn Device>>>, IpAddr, Mbuf)> {
        let tx = match self.tx.pop_front() {
            Some(tx) => tx,
            None => return None,
        };

        let mut mbuf = Mbuf::new();
        mbuf.prepend(&UdpHeader {
            dst_port: tx.dst_port.as_u16().into(),
            src_port: self.bind_to.port.as_u16().into(),
            len: ((size_of::<UdpHeader>() + tx.payload.len()) as u16).into(),
            checksum: 0.into(),
        });
        mbuf.append_bytes(tx.payload.as_slice());

        Some((tx.device, tx.dst_addr, mbuf))
    }

    fn receive<'a>(&mut self, src_addr: IpAddr, _dst_addr: IpAddr, header: &TransportHeader<'a>) {
        let header = match header {
            TransportHeader::Udp(header) => header,
            _ => unreachable!(),
        };

        let rx_data = RxPacket {
            src_addr,
            src_port: header.src_port,
            payload: Vec::from(header.payload),
        };

        self.rx.push_back(rx_data);
    }

    fn send(
        &mut self,
        device: Option<Rc<RefCell<dyn Device>>>,
        dst_addr: IpAddr,
        dst_port: Port,
        payload: &[u8],
    ) {
        self.tx.push_back(TxPacket {
            device,
            dst_addr,
            dst_port,
            payload: payload.to_vec(),
        });
    }

    fn recv(&mut self) -> Option<(IpAddr, Port, Vec<u8>)> {
        match self.rx.pop_front() {
            Some(pkt) => Some((pkt.src_addr, pkt.src_port, pkt.payload)),
            None => None,
        }
    }

    fn close(&mut self) {
        unreachable!();
    }

    fn readable_len(&mut self) -> usize {
        unreachable!();
    }

    fn read(&mut self, _buf: &mut [u8]) -> usize {
        unreachable!();
    }

    fn write(&mut self, _data: &[u8]) -> Result<()> {
        unreachable!();
    }

    fn accept(&mut self) -> Option<TcpSocket> {
        unreachable!();
    }

    fn protocol(&self) -> TransportProtocol {
        TransportProtocol::Udp
    }

    fn bind_to(&self) -> BindTo {
        self.bind_to
    }
}

#[repr(C, packed)]
struct UdpHeader {
    src_port: NetEndian<u16>,
    dst_port: NetEndian<u16>,
    len: NetEndian<u16>,
    checksum: NetEndian<u16>,
}

pub fn parse<'a>(pkt: &'a mut Packet) -> Option<(Port, Port, TransportHeader<'a>)> {
    let header = match pkt.consume::<UdpHeader>() {
        Some(header) => header,
        None => return None,
    };

    let dst_port = Port::new(header.dst_port.into());
    let src_port = Port::new(header.src_port.into());
    let total_len: u16 = header.len.into();
    let payload_len = (total_len as usize) - size_of::<UdpHeader>();
    let payload = unwrap_or_return!(pkt.consume_bytes(payload_len), None);

    let parsed_header = TransportHeader::Udp(UdpTransportHeader { src_port, payload });

    Some((src_port, dst_port, parsed_header))
}
