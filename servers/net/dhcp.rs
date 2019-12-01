use crate::device::{Device, MacAddr};
use crate::endian::{swap32, NetEndian};
use crate::ip::IpAddr;
use crate::ipv4::Ipv4Addr;
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use crate::transport::Port;
use resea::cell::RefCell;
use resea::rc::Rc;

pub enum DhcpClientState {
    SendingDiscovery,
    WaitingOffer,
    SendingRequest(Ipv4Addr),
    WaitingAck,
    Done,
}

pub struct DhcpClient {
    state: DhcpClientState,
    device: Rc<RefCell<dyn Device>>,
}

impl DhcpClient {
    pub fn new(device: Rc<RefCell<dyn Device>>) -> DhcpClient {
        DhcpClient {
            state: DhcpClientState::SendingDiscovery,
            device,
        }
    }

    pub fn device(&self) -> &Rc<RefCell<dyn Device>> {
        &self.device
    }

    pub fn build(&mut self) -> Option<(IpAddr, Mbuf)> {
        match self.state {
            DhcpClientState::SendingDiscovery => {
                let param_list = [
                    OPTION_SUBNET_MASK,
                    OPTION_ROUTER,
                ];
                let mut mbuf = Mbuf::new();
                build(
                    &mut mbuf,
                    DHCP_TYPE_DISCOVER,
                    self.device.borrow().mac_addr(),
                    None,
                    Some(&param_list),
                );
                self.state = DhcpClientState::WaitingOffer;
                Some((IpAddr::Ipv4(Ipv4Addr::BROADCAST), mbuf))
            }
            DhcpClientState::SendingRequest(request_ipaddr) => {
                let mut mbuf = Mbuf::new();
                build(
                    &mut mbuf,
                    DHCP_TYPE_REQUEST,
                    self.device.borrow().mac_addr(),
                    Some(request_ipaddr),
                    None,
                );
                self.state = DhcpClientState::WaitingAck;
                Some((IpAddr::Ipv4(Ipv4Addr::BROADCAST), mbuf))
            }
            _ => None,
        }
    }

    pub fn receive(
        &mut self,
        _src_addr: IpAddr,
        _src_port: Port,
        payload: &[u8],
    ) -> Option<(Ipv4Addr, Option<Ipv4Addr>, u32)> {
        let mut pkt = Packet::new(payload);
        if let Some(parsed) = parse(&mut pkt) {
            match self.state {
                DhcpClientState::WaitingOffer if parsed.dhcp_type == DHCP_TYPE_OFFER => {
                    info!("DHCP offer: {}", parsed.your_ipaddr);
                    self.state = DhcpClientState::SendingRequest(parsed.your_ipaddr);
                }
                DhcpClientState::WaitingAck if parsed.dhcp_type == DHCP_TYPE_ACK => {
                    info!("DHCP ack: {}", parsed.your_ipaddr);
                    let netmask = unwrap_or_return!(parsed.netmask, None);
                    self.state = DhcpClientState::Done;
                    return Some((parsed.your_ipaddr, parsed.router, netmask));
                }
                _ => {}
            }
        }

        None
    }
}

#[repr(C, packed)]
struct DhcpPacket {
    op: u8,
    hw_type: u8,
    hw_len: u8,
    hops: u8,
    xid: NetEndian<u32>,
    secs: NetEndian<u16>,
    flags: NetEndian<u16>,
    client_ipaddr: NetEndian<u32>,
    your_ipaddr: NetEndian<u32>,
    server_ipaddr: NetEndian<u32>,
    gateway_ipaddr: NetEndian<u32>,
    client_hwaddr: [u8; 6],
    _unused: [u8; 202],
    magic: NetEndian<u32>,
    options: [u8; 0],
}

const BOOTP_OP_REQUEST: u8 = 1;
const BOOTP_HWTYPE_ETHERNET: u8 = 1;
const DHCP_MAGIC: u32 = 0x6382_5363;
const OPTION_DHCP_TYPE: u8 = 53;
const DHCP_TYPE_DISCOVER: u8 = 1;
const DHCP_TYPE_OFFER: u8 = 2;
const DHCP_TYPE_REQUEST: u8 = 3;
const DHCP_TYPE_ACK: u8 = 5;
const OPTION_REQUESTED_IP_ADDR: u8 = 50;
const OPTION_PARAM_REQUEST_LIST: u8 = 55;
const OPTION_SUBNET_MASK: u8 = 1;
const OPTION_ROUTER: u8 = 3;
const OPTION_END: u8 = 255;

pub fn build(
    pkt: &mut Mbuf,
    dhcp_type: u8,
    client_hwaddr: MacAddr,
    requested_ipaddr: Option<Ipv4Addr>,
    param_list: Option<&[u8]>,
) {
    pkt.append(&DhcpPacket {
        op: BOOTP_OP_REQUEST,
        hw_type: BOOTP_HWTYPE_ETHERNET,
        hw_len: 6,
        hops: 0,
        xid: 0x1234_5678.into(),
        secs: 0.into(),
        flags: 0.into(),
        client_ipaddr: 0.into(),
        your_ipaddr: 0.into(),
        server_ipaddr: 0.into(),
        gateway_ipaddr: 0.into(),
        client_hwaddr: *client_hwaddr.as_slice(),
        _unused: [0; 202],
        magic: DHCP_MAGIC.into(),
        options: [],
    });

    // DHCP message type.
    pkt.append::<u8>(&OPTION_DHCP_TYPE);
    pkt.append::<u8>(&1);
    pkt.append::<u8>(&dhcp_type);

    // Requested IP Address.
    if let Some(ipaddr) = requested_ipaddr {
        pkt.append::<u8>(&OPTION_REQUESTED_IP_ADDR);
        pkt.append::<u8>(&4);
        pkt.append::<u32>(&swap32(ipaddr.as_u32()));
    }

    // Parameter request list.
    if let Some(params) = param_list {
        pkt.append::<u8>(&OPTION_PARAM_REQUEST_LIST);
        pkt.append::<u8>(&(params.len() as u8));
        for param in params {
            pkt.append::<u8>(&param);
        }
    }

    // The end of options.
    pkt.append::<u8>(&OPTION_END);

    // Padding.
    let padding_len = 4 - (pkt.len() % 4) + 8;
    for _ in 0..padding_len {
        pkt.append::<u8>(&0x00);
    }
}

struct ParsedDhcpPacket {
    dhcp_type: u8,
    your_ipaddr: Ipv4Addr,
    router: Option<Ipv4Addr>,
    netmask: Option<u32>,
}

fn parse<'a>(pkt: &'a mut Packet) -> Option<ParsedDhcpPacket> {
    let header = unwrap_or_return!(pkt.consume::<DhcpPacket>(), None);
    if Into::<u32>::into(header.magic) != DHCP_MAGIC {
        return None;
    }

    let your_ipaddr = Ipv4Addr::from_u32(header.your_ipaddr.into());

    // Parse options.
    let mut dhcp_type = None;
    let mut router = None;
    let mut netmask: Option<u32> = None;
    loop {
        let option_type: u8 = *unwrap_or_return!(pkt.consume::<u8>(), None);
        let option_len: u8 = *unwrap_or_return!(pkt.consume::<u8>(), None);

        match option_type {
            OPTION_DHCP_TYPE => {
                dhcp_type = Some(*unwrap_or_return!(pkt.consume::<u8>(), None));
            }
            OPTION_SUBNET_MASK => {
                let value = *unwrap_or_return!(pkt.consume::<u32>(), None);
                netmask = Some(NetEndian::new(value).into());
            }
            OPTION_ROUTER => {
                let value = *unwrap_or_return!(pkt.consume::<u32>(), None);
                router = Some(Ipv4Addr::from_u32(NetEndian::new(value).into()));
            }
            OPTION_END => {
                break;
            }
            _ => {
                unwrap_or_return!(pkt.consume_bytes(option_len as usize), None);
            }
        }
    }

    match dhcp_type {
        Some(DHCP_TYPE_OFFER) => Some(ParsedDhcpPacket {
            dhcp_type: DHCP_TYPE_OFFER,
            your_ipaddr,
            router,
            netmask,
        }),
        Some(DHCP_TYPE_ACK) => Some(ParsedDhcpPacket {
            dhcp_type: DHCP_TYPE_ACK,
            your_ipaddr,
            router,
            netmask,
        }),
        Some(_) | None => None,
    }
}
