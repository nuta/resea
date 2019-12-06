use crate::device::{Device, MacAddr};
use crate::dhcp::DhcpClient;
use crate::ethernet::EthernetDevice;
use crate::ip::{IpAddr, NetworkProtocol, Route};
use crate::ipv4::{self, Ipv4Addr, Ipv4Network};
use crate::mbuf::Mbuf;
use crate::packet::Packet;
use crate::tcp::{self, TcpSocket};
use crate::transport::{BindTo, Port, Socket, TransportProtocol};
use crate::udp::{self, UdpSocket};
use crate::{Error, Result};
use resea::cell::RefCell;
use resea::collections::{HashMap, VecDeque};
use resea::rc::Rc;
use resea::string::String;
use resea::vec::Vec;

pub struct TcpIp {
    routes: Vec<Route>,
    tx_queue: RefCell<VecDeque<Mbuf>>,
    devices: HashMap<String, Rc<RefCell<dyn Device>>>,
    sockets: HashMap<(BindTo, BindTo), Rc<RefCell<dyn Socket>>>,
    dhcp_client: Option<(SocketHandle, DhcpClient)>,
}

#[derive(Clone)]
pub struct SocketHandle(Rc<RefCell<dyn Socket>>);

impl SocketHandle {
    pub(crate) fn new(sock: Rc<RefCell<dyn Socket>>) -> SocketHandle {
        SocketHandle(sock)
    }
}

impl PartialEq for SocketHandle {
    fn eq(&self, other: &SocketHandle) -> bool {
        Rc::ptr_eq(&self.0, &other.0)
    }
}

pub enum DeviceIpAddr {
    #[allow(unused)]
    Static(Ipv4Addr),
    Dhcp,
}

impl TcpIp {
    pub fn new() -> TcpIp {
        TcpIp {
            routes: Vec::new(),
            devices: HashMap::new(),
            sockets: HashMap::new(),
            tx_queue: RefCell::new(VecDeque::new()),
            dhcp_client: None,
        }
    }

    pub fn add_ethernet_device(
        &mut self,
        name: &str,
        mac_addr: MacAddr,
        device_ip_addr: DeviceIpAddr,
    ) {
        use resea::borrow::ToOwned;
        let device = Rc::new(RefCell::new(EthernetDevice::new(mac_addr)));
        match device_ip_addr {
            DeviceIpAddr::Static(addr) => device.borrow_mut().set_ipv4_addr(addr),
            DeviceIpAddr::Dhcp => {
                let sock = self.udp_open(IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED), Port::new(68));
                let dhcp_client = DhcpClient::new(device.clone());
                assert!(self.dhcp_client.is_none());
                self.dhcp_client = Some((sock, dhcp_client));
                info!("created a dhcp client");
            }
        }

        self.devices.insert(name.to_owned(), device);
    }

    pub fn add_route(&mut self, device: &str, ipv4_network: Ipv4Network, gateway: Option<IpAddr>, our_ipv4_addr: Ipv4Addr) {
        let device = self.devices.get(device).cloned().expect("unknown device");
        self.routes
            .push(Route::new(ipv4_network, gateway, our_ipv4_addr, device));
    }

    pub fn pop_tx_packet(&mut self) -> Option<Mbuf> {
        self.tx_queue.borrow_mut().pop_front()
    }

    pub fn tcp_listen(&mut self, port: Port) -> SocketHandle {
        let addr = IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED);
        let sock = Rc::new(RefCell::new(TcpSocket::new_listen(addr, port)));
        let local = BindTo::new(TransportProtocol::Tcp, addr, port);
        let remote = BindTo::new(
            TransportProtocol::Tcp,
            IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED),
            Port::UNSPECIFIED,
        );
        assert!(self.sockets.get(&(local, remote)).is_none());
        self.sockets.insert((local, remote), sock.clone());
        SocketHandle::new(sock)
    }

    pub fn tcp_close(&mut self, sock: &SocketHandle) {
        sock.0.borrow_mut().close();
    }

    pub fn tcp_write(&mut self, sock: &SocketHandle, data: &[u8]) -> Result<()> {
        sock.0.borrow_mut().write(data)
    }

    pub fn tcp_readable_len(&mut self, sock: &SocketHandle) -> usize {
        sock.0.borrow_mut().readable_len()
    }

    pub fn tcp_read(&mut self, sock: &SocketHandle, buf: &mut [u8]) -> usize {
        sock.0.borrow_mut().read(buf)
    }

    pub fn tcp_accept(&mut self, sock: &SocketHandle) -> Option<SocketHandle> {
        sock.0.borrow_mut().accept().map(|tcp_sock| {
            let mut local = tcp_sock.bind_to();
            local.addr = IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED); // FIXME:
            let remote = BindTo::new(
                TransportProtocol::Tcp,
                tcp_sock.remote_addr().unwrap(),
                tcp_sock.remote_port().unwrap(),
            );
            let sock = Rc::new(RefCell::new(tcp_sock));
            self.sockets.insert((local, remote), sock.clone());
            SocketHandle::new(sock)
        })
    }

    pub fn udp_open(&mut self, addr: IpAddr, port: Port) -> SocketHandle {
        let sock = Rc::new(RefCell::new(UdpSocket::new(BindTo::new(
            TransportProtocol::Udp,
            addr,
            port,
        ))));
        let local = BindTo::new(TransportProtocol::Udp, addr, port);
        let remote = BindTo::new(
            TransportProtocol::Udp,
            IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED),
            Port::UNSPECIFIED,
        );
        assert!(self.sockets.get(&(local, remote)).is_none());
        self.sockets.insert((local, remote), sock.clone());
        SocketHandle::new(sock)
    }

    pub fn _udp_send(
        &mut self,
        sock: &SocketHandle,
        dst_addr: IpAddr,
        dst_port: Port,
        payload: &[u8],
    ) {
        sock.0.borrow_mut().send(None, dst_addr, dst_port, payload);
    }

    pub fn _udp_recv(&mut self, sock: &SocketHandle) -> Option<(IpAddr, Port, Vec<u8>)> {
        sock.0.borrow_mut().recv()
    }

    pub fn receive(&mut self, device_name: &str, pkt: &[u8]) -> Option<SocketHandle> {
        let device = unwrap_or_return!(self.devices.get(device_name), None);
        let mut pkt = Packet::new(pkt);
        let net_type = unwrap_or_return!(
            device
                .borrow_mut()
                .receive(&mut *self.tx_queue.borrow_mut(), &mut pkt),
            None
        );
        drop(device);

        let sock = match net_type {
            NetworkProtocol::Ipv4 =>
                self.receive_ipv4_packet(pkt)
                    .map(|inner| SocketHandle::new(inner.clone())),
        };

        // Handle DHCP messages.
        match (sock, &mut self.dhcp_client) {
            (Some(sock), Some(client)) if client.0 == sock => {
                if let Some((src_addr, src_port, payload)) = sock.0.borrow_mut().recv() {
                    if let Some((our_addr, gateway, netmask)) = client.1.receive(src_addr, src_port, &payload) {
                        info!("DHCP: our_ip={} gateway={:?} netmask={:x}",
                            our_addr, gateway, netmask);
                        let device = unwrap_or_return!(self.devices.get(device_name), None);
                        device.borrow_mut().set_ipv4_addr(our_addr);
                        self.add_route(
                            device_name,
                            Ipv4Network::from_ipv4_addr(our_addr, netmask),
                            None,
                            our_addr,
                        );
                        if let Some(gateway) = gateway {
                            self.add_route(
                                device_name,
                                Ipv4Network::UNSPECIFIED,
                                Some(IpAddr::Ipv4(gateway)),
                                our_addr,
                            );
                        }
                    }
                }
                None
            }
            (Some(sock), _) => Some(sock),
            (None, _) => None,
        }
    }

    pub fn interval_work(&mut self) -> Result<()> {
        // Send a DHCP message if needed.
        if let Some((sock, client)) = &mut self.dhcp_client {
            if let Some((addr, pkt)) = client.build() {
                let device = client.device().clone();
                sock.0
                    .borrow_mut()
                    .send(Some(device), addr, Port::new(67), pkt.as_bytes());
            }
        }

        // Send pending packets.
        loop {
            let mut num_sent_packets = 0;
            for (_, sock) in self.sockets.iter() {
                let mut sock = sock.borrow_mut();
                let (device, dst, pkt) = match sock.build() {
                    Some((device, dst, pkt)) => (device, dst, pkt),
                    None => {
                        // The socket no longer have data to send. Remove it from
                        // the queue.
                        continue;
                    }
                };

                match sock.bind_to().addr {
                    IpAddr::Ipv4(_) => self.send_ipv4_packet(dst, sock.protocol(), device, pkt)?,
                }

                num_sent_packets += 1;
            }

            if num_sent_packets == 0 {
                // We've sent all pending packets.
                break;
            }
        }

        Ok(())
    }

    fn send_ipv4_packet(
        &self,
        dst: IpAddr,
        proto: TransportProtocol,
        device: Option<Rc<RefCell<dyn Device>>>,
        mut pkt: Mbuf,
    ) -> Result<()> {
        let (device, next_addr, src_addr) = match device {
            Some(device) => (device, dst, Ipv4Addr::UNSPECIFIED),
            None => {
                match self.look_for_route(&dst) {
                    Some(route) =>
                        (route.device.clone(), route.gateway.unwrap_or(dst), route.our_ipv4_addr),
                    None => {
                        return Err(Error::NoRoute)
                    }
                }
            }
        };

        let len = pkt.len();
        match dst {
            IpAddr::Ipv4(dst) => ipv4::build(&mut pkt, dst, src_addr, proto, len),
        }

        let mut queue = self.tx_queue.borrow_mut();
        device.borrow_mut().enqueue(&mut *queue, next_addr, pkt);
        Ok(())
    }

    fn receive_ipv4_packet<'a>(&mut self, mut pkt: Packet<'a>) -> Option<&Rc<RefCell<dyn Socket>>> {
        let (mut dst_addr, src_addr, trans, len) =
            unwrap_or_return!(ipv4::parse(&mut pkt), None);
        let src = IpAddr::Ipv4(src_addr);
        let dst = IpAddr::Ipv4(dst_addr);

        let (src_port, dst_port, parsed_trans_data) = match trans {
            TransportProtocol::Udp => unwrap_or_return!(udp::parse(&mut pkt), None),
            TransportProtocol::Tcp => unwrap_or_return!(tcp::parse(&mut pkt, len), None),
        };

        let local = BindTo::new(trans, IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED), dst_port);
        let remote = BindTo::new(trans, src, src_port);

        if src_port.as_u16() == 67 && dst_port.as_u16() == 68 {
            // XXX: Overwrite the destination address if the packet is a DHCP
            //      message from the server.
            // https://superuser.com/questions/1090457/dhcp-offer-already-sent-to-my-local-ip-address
            dst_addr = Ipv4Addr::BROADCAST;
        }

        if dst_addr != Ipv4Addr::BROADCAST && !self.is_our_ip_addr(&dst) {
            return None;
        }

        // Look for the socket.
        let unspecified = BindTo::new(
            trans,
            IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED),
            Port::UNSPECIFIED,
        );
        let mut sock = self.sockets.get(&(local, remote));
        if sock.is_none() {
            sock = self.sockets.get(&(local, unspecified));
        }
        if sock.is_none() {
            sock = self.sockets.get(&(unspecified, unspecified))
        }

        match sock {
            Some(sock) => {
                sock.borrow_mut().receive(src, dst, &parsed_trans_data);
                Some(sock)
            }
            None => None,
        }
    }

    fn look_for_route(&self, addr: &IpAddr) -> Option<&Route> {
        for route in &self.routes {
            if route.ipv4_network.netmask() == 0 {
                continue;
            }

            match addr {
                IpAddr::Ipv4(addr) if route.ipv4_network.contains(*addr) => {
                    return Some(route);
                }
                _ => (),
            }
        }

        // Use the default gateway.
        for route in &self.routes {
            if route.ipv4_network.netmask() == 0 {
                return Some(route);
            }
        }

        // No routes found.
        None
    }

    fn is_our_ip_addr(&self, addr: &IpAddr) -> bool {
        for route in &self.routes {
            match addr {
                IpAddr::Ipv4(addr) if route.our_ipv4_addr == *addr => {
                    return true;
                }
                _ => (),
            }
        }

        false
    }
}
