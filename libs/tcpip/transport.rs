use crate::device::Device;
use crate::ip::IpAddr;
use crate::mbuf::Mbuf;
use crate::tcp::{TcpFlags, TcpSocket};
use crate::Result;
use resea::cell::RefCell;
use resea::fmt;
use resea::rc::Rc;
use resea::vec::Vec;

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Port(u16);

impl Port {
    pub const UNSPECIFIED: Port = Port::new(0);

    pub const fn new(value: u16) -> Port {
        Port(value)
    }

    pub fn as_u16(&self) -> u16 {
        self.0
    }
}

impl fmt::Display for Port {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub enum TransportProtocol {
    Tcp,
    Udp,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct BindTo {
    pub addr: IpAddr,
    pub trans: TransportProtocol,
    pub port: Port,
}

impl BindTo {
    pub const fn new(trans: TransportProtocol, addr: IpAddr, port: Port) -> BindTo {
        BindTo { trans, addr, port }
    }
}

#[derive(Debug)]
pub struct TcpTransportHeader<'a> {
    pub src_port: Port,
    pub payload: &'a [u8],
    pub flags: TcpFlags,
    pub ack_no: u32,
    pub seq_no: u32,
    pub win_size: u16,
}

#[derive(Debug)]
pub struct UdpTransportHeader<'a> {
    pub src_port: Port,
    pub payload: &'a [u8],
}

#[derive(Debug)]
pub enum TransportHeader<'a> {
    Tcp(TcpTransportHeader<'a>),
    Udp(UdpTransportHeader<'a>),
}

pub trait Socket {
    fn build(&mut self) -> Option<(Option<Rc<RefCell<dyn Device>>>, IpAddr, Mbuf)>;
    fn close(&mut self);
    fn receive<'a>(&mut self, src_addr: IpAddr, dst_addr: IpAddr, header: &TransportHeader<'a>);
    fn protocol(&self) -> TransportProtocol;
    fn bind_to(&self) -> BindTo;

    // TCP only.
    fn write(&mut self, data: &[u8]) -> Result<()>;
    fn read(&mut self, buf: &mut Vec<u8>, len: usize) -> usize;
    fn accept(&mut self) -> Option<TcpSocket>;

    // UDP only.
    fn send(
        &mut self,
        device: Option<Rc<RefCell<dyn Device>>>,
        dst_addr: IpAddr,
        dst_port: Port,
        payload: &[u8],
    );
    fn recv(&mut self) -> Option<(IpAddr, Port, Vec<u8>)>;
}
