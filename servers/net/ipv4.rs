use crate::checksum::Checksum;
use crate::endian::NetEndian;
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use crate::transport::TransportProtocol;
use resea::fmt;
use resea::mem::size_of;

#[repr(transparent)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct Ipv4Addr(u32);

impl Ipv4Addr {
    pub const UNSPECIFIED: Ipv4Addr = Ipv4Addr::new(0, 0, 0, 0);
    pub const BROADCAST: Ipv4Addr = Ipv4Addr::new(255, 255, 255, 255);

    pub const fn from_u32(addr: u32) -> Ipv4Addr {
        Ipv4Addr(addr)
    }

    pub const fn new(a0: u8, a1: u8, a2: u8, a3: u8) -> Ipv4Addr {
        Ipv4Addr(((a0 as u32) << 24) | ((a1 as u32) << 16) | ((a2 as u32) << 8) | (a3 as u32))
    }

    pub const fn as_u32(self) -> u32 {
        self.0
    }
}

impl fmt::Display for Ipv4Addr {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(
            f,
            "{}.{}.{}.{}",
            self.0 >> 24,
            (self.0 >> 16) & 0xff,
            (self.0 >> 8) & 0xff,
            self.0 & 0xff
        )
    }
}

#[derive(Clone, Copy, Debug)]
pub struct Ipv4Network {
    addr: u32,
    netmask: u32,
}

impl Ipv4Network {
    pub const UNSPECIFIED: Ipv4Network = Ipv4Network::new(0, 0, 0, 0, 0);

    pub const fn new(a0: u8, a1: u8, a2: u8, a3: u8, netmask: u32) -> Ipv4Network {
        Ipv4Network {
            addr: ((a0 as u32) << 24) | ((a1 as u32) << 16) | ((a2 as u32) << 8) | (a3 as u32),
            netmask,
        }
    }

    pub const fn from_ipv4_addr(addr: Ipv4Addr, netmask: u32) -> Ipv4Network {
        Ipv4Network {
            addr: addr.as_u32(),
            netmask
        }
    }

    pub fn netmask(self) -> u32 {
        self.netmask
    }

    pub fn contains(self, addr: Ipv4Addr) -> bool {
        (self.addr & self.netmask) == (addr.as_u32() & self.netmask)
    }
}

#[repr(C, packed)]
struct Ipv4Header {
    ver_ihl: u8,
    dscp_ecn: u8,
    len: NetEndian<u16>,
    id: NetEndian<u16>,
    flags_frag_off: NetEndian<u16>,
    ttl: u8,
    proto: u8,
    checksum: NetEndian<u16>,
    src_addr: NetEndian<u32>,
    dst_addr: NetEndian<u32>,
}

pub const IPV4_PROTO_TCP: u8 = 0x06;
const IPV4_PROTO_UDP: u8 = 0x11;
const DEFAULT_TTL: u8 = 32;

pub fn parse(pkt: &mut Packet) -> Option<(Ipv4Addr, Ipv4Addr, TransportProtocol, usize)> {
    let header = match pkt.consume::<Ipv4Header>() {
        Some(header) => header,
        None => return None,
    };

    let dst = Ipv4Addr::from_u32(header.dst_addr.into());
    let src = Ipv4Addr::from_u32(header.src_addr.into());
    let total_len: u16 = header.len.into();
    let header_len = (header.ver_ihl & 0x0f) * 4;
    let payload_len = (total_len as usize) - (header_len as usize);
    let trans = match header.proto {
        IPV4_PROTO_TCP => TransportProtocol::Tcp,
        IPV4_PROTO_UDP => TransportProtocol::Udp,
        _ => return None,
    };

    Some((dst, src, trans, payload_len))
}

pub fn build(mbuf: &mut Mbuf, dst: Ipv4Addr, src: Ipv4Addr, proto: TransportProtocol, len: usize) {
    let proto = match proto {
        TransportProtocol::Tcp => IPV4_PROTO_TCP,
        TransportProtocol::Udp => IPV4_PROTO_UDP,
    };

    let header_len = size_of::<Ipv4Header>();
    let mut header = Ipv4Header {
        ver_ihl: 0x45,
        dscp_ecn: 0,
        len: ((header_len + len) as u16).into(),
        id: 0.into(),
        flags_frag_off: 0.into(),
        ttl: DEFAULT_TTL,
        proto,
        checksum: 0.into(),
        src_addr: src.as_u32().into(),
        dst_addr: dst.as_u32().into(),
    };

    let mut checksum = Checksum::new();
    checksum.input_struct(&header);
    header.checksum = checksum.finish().into();

    mbuf.prepend(&header);
}
