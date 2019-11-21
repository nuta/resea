use resea::std::rc::Rc;
use resea::std::cell::RefCell;
use resea::std::string::String;
use resea::collections::Vec;
use resea::collections::{HashMap, LinkedList, VecDeque};
use crate::{Result, Error};
use crate::device::Device;
use crate::ethernet::{MacAddr, EthernetDevice};
use crate::ip::{IpAddr, Route, NetworkProtocol};
use crate::ipv4::{self, Ipv4Addr, Ipv4Network};
use crate::transport::{Socket, BindTo, Port, TransportProtocol};
use crate::ethernet;
use crate::udp::{self, UdpSocket};
use crate::tcp::{self, TcpSocket};
use crate::mbuf::Mbuf;
use crate::packet::Packet;

pub struct Instance {
    routes: Vec<Route>,
    tx_queue: RefCell<VecDeque<Mbuf>>,
    devices: HashMap<String, Rc<RefCell<dyn Device>>>,
    sockets: HashMap<(BindTo, BindTo), Rc<RefCell<dyn Socket>>>,
}

pub struct SocketHandle(Rc<RefCell<dyn Socket>>);
impl SocketHandle {
    pub(crate) fn new(sock: Rc<RefCell<dyn Socket>>) -> SocketHandle {
        SocketHandle(sock)
    }
}

impl Instance {
    pub fn new() -> Instance {
        Instance {
            routes: Vec::new(),
            devices: HashMap::new(),
            sockets: HashMap::new(),
            tx_queue: RefCell::new(VecDeque::new()),
        }
    }

    pub fn add_ethernet_device(&mut self, name: &str, mac_addr: MacAddr, ipv4_addr: Option<Ipv4Addr>) {
        use resea::std::borrow::ToOwned;
        let mut device = EthernetDevice::new(mac_addr);
        if let Some(addr) = ipv4_addr {
            device.set_ipv4_addr(addr);
        }

        self.devices.insert(name.to_owned(), Rc::new(RefCell::new(device)));
    }

    pub fn add_route(&mut self, device: &str, ipv4_network: Ipv4Network, our_ipv4_addr: Ipv4Addr) {
        let device = self.devices.get(device).cloned().expect("unknown device");
        self.routes.push(Route::new(ipv4_network, our_ipv4_addr, device));
    }

    pub fn pop_tx_packet(&mut self) -> Option<Mbuf> {
        self.tx_queue.borrow_mut().pop_front()
    }

    pub fn tcp_listen(&mut self, addr: IpAddr, port: Port) -> SocketHandle {
        let sock = Rc::new(RefCell::new(TcpSocket::new_listen(addr, port)));
        let local = BindTo::new(TransportProtocol::Tcp, addr, port);
        let remote = BindTo::new(TransportProtocol::Tcp, IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED), Port::UNSPECIFIED);
        assert!(self.sockets.get(&(local, remote)).is_none());
        self.sockets.insert((local, remote), sock.clone());
        SocketHandle::new(sock)
    }

    pub fn tcp_close(&mut self, sock: &SocketHandle) {
        sock.0.borrow_mut().close();
    }

    pub fn tcp_write(&mut self, sock: &SocketHandle, data: &[u8]) {
        sock.0.borrow_mut().write(data);
    }

    pub fn tcp_read(&mut self, sock: &SocketHandle, buf: &mut Vec<u8>, len: usize) -> usize {
        sock.0.borrow_mut().read(buf, len)
    }

    pub fn tcp_accept(&mut self, sock: &SocketHandle) -> Option<SocketHandle> {
        sock.0.borrow_mut()
            .accept()
            .map(|tcp_sock| {
                let local = tcp_sock.bind_to().clone();
                let remote = BindTo::new(TransportProtocol::Tcp, tcp_sock.remote_addr().unwrap(), tcp_sock.remote_port().unwrap());
                let sock = Rc::new(RefCell::new(tcp_sock));
                self.sockets.insert((local, remote), sock.clone());
                SocketHandle::new(sock)
            })
    }

    pub fn receive(&mut self, device_name: &str, pkt: &[u8]) -> Option<SocketHandle> {
        let device = unwrap_or_return!(self.devices.get(device_name), None);
        let mut pkt = Packet::new(pkt);
        let net_type =
            unwrap_or_return!(
                device.borrow_mut().receive(&mut *self.tx_queue.borrow_mut(), &mut pkt),
                None);

        match net_type {
            NetworkProtocol::Ipv4 =>
                self.receive_ipv4_packet(pkt).map(|inner| SocketHandle::new(inner.clone())),
        }
    }

    pub fn interval_work(&mut self) -> Result<()> {
        // Send pending packets.
        loop {
            let mut num_sent_packets = 0;

            for (_, sock) in self.sockets.iter() {
                let mut sock = sock.borrow_mut();
                let (dst, pkt) = match sock.build() {
                    Some((dst, pkt)) => (dst, pkt),
                    None => {
                        // The socket no longer have data to send. Remove it from
                        // the queue.
                        continue;
                    }
                };

                match sock.bind_to().addr {
                    IpAddr::Ipv4(_) =>
                        self.send_ipv4_packet(dst, sock.protocol(), pkt)?,
                }

                num_sent_packets += 1;
            }

            trace!("sent {} packets", num_sent_packets);
            if num_sent_packets == 0 {
                // We've sent all pending packets.
                break;
            }
        }

        Ok(())
    }

    fn send_ipv4_packet(&self, dst: IpAddr, proto: TransportProtocol, mut pkt: Mbuf) -> Result<()> {
        let route = match self.look_for_route(&dst) {
            Some(route) => route,
            None => {
                return Err(Error::NoRoute);
            }
        };

        let len = pkt.len();
        match dst {
            IpAddr::Ipv4(dst) =>
                ipv4::build(&mut pkt, dst, route.our_ipv4_addr, proto, len),
        }

        let mut queue = self.tx_queue.borrow_mut();
        route.device.borrow_mut().enqueue(&mut *queue, dst, pkt);
        Ok(())
    }

    fn receive_ipv4_packet<'a>(&mut self, mut pkt: Packet<'a>) -> Option<&Rc<RefCell<dyn Socket>>> {
        let (dst_addr, src_addr, trans, len) =
            unwrap_or_return!(ipv4::parse(&mut pkt), None);
        let src = IpAddr::Ipv4(src_addr);
        let dst = IpAddr::Ipv4(dst_addr);
        if !self.is_our_ip_addr(&dst) {
            return None;
        }

        let (src_port, dst_port, parsed_trans_data) = match trans {
            TransportProtocol::Udp => unwrap_or_return!(udp::parse(&mut pkt), None),
            TransportProtocol::Tcp => unwrap_or_return!(tcp::parse(&mut pkt, len), None),
        };

        let local = BindTo::new(trans, dst, dst_port);
        let remote = BindTo::new(trans, src, src_port);
        info!("ipv4 packet: {:?} -> {:?}", remote, local);
        if let Some(sock) = self.sockets.get(&(local, remote)) {
            sock.borrow_mut().receive(src, &parsed_trans_data);
            Some(sock)
        } else {
            let remote = BindTo::new(trans, IpAddr::Ipv4(Ipv4Addr::UNSPECIFIED), Port::UNSPECIFIED);
            if let Some(sock) = self.sockets.get(&(local, remote)) {
                sock.borrow_mut().receive(src, &parsed_trans_data);
                Some(sock)
            } else {
                None
            }
        }
    }

    fn look_for_route(&self, addr: &IpAddr) -> Option<&Route> {
        for route in &self.routes {
            match addr {
                IpAddr::Ipv4(addr) if route.ipv4_network.contains(*addr) => {
                    return Some(route);
                }
                _ => (),
            }
        }

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
