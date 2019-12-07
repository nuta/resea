use resea::collections::HashMap;
use resea::idl;
use resea::idl::net_client::{nbsend_tcp_accepted, nbsend_tcp_received};
use resea::idl::network_device::{call_get_macaddr, call_transmit};
use resea::prelude::*;
use resea::rc::Rc;
use resea::server::{connect_to_server, publish_server};
use resea::mainloop::DeferredWorkResult;
use resea::utils::align_up;
use resea::PAGE_SIZE;
use crate::device::MacAddr;
use crate::tcpip::{DeviceIpAddr, TcpIp, SocketHandle};
use crate::transport::Port;

static MEMMGR_SERVER: Channel = Channel::from_cid(1);
static _KERNEL_SERVER: Channel = Channel::from_cid(2);

struct TcpSocket {
    handle: HandleId,
    sock: SocketHandle,
    listener: Rc<Channel>,
}

struct Server {
    ch: Channel,
    network_device: Channel,
    tcpip: TcpIp,
    sockets: HashMap<HandleId, Rc<TcpSocket>>,
    tcp_sockets: Vec<Rc<TcpSocket>>,
    accepted_queue: Vec<(Rc<Channel>, HandleId, HandleId)>,
    received_queue: Vec<(Rc<Channel>, HandleId, Page)>,
    next_sock_id: i32,
}

impl Server {
    pub fn new(server_ch: Channel, network_device: Channel) -> Server {
        let mut tcpip = TcpIp::new();
        let ma = call_get_macaddr(&network_device).unwrap();
        let macaddr = MacAddr::new([ma.0, ma.1, ma.2, ma.3, ma.4, ma.5]);
        tcpip.add_ethernet_device("net0", macaddr, DeviceIpAddr::Dhcp);
        tcpip.interval_work().unwrap(); // TODO: remove
        Server {
            ch: server_ch,
            network_device,
            tcpip,
            sockets: HashMap::new(),
            tcp_sockets: Vec::new(),
            accepted_queue: Vec::new(),
            received_queue: Vec::new(),
            next_sock_id: 1,
        }
    }

    pub fn flush_queued_messages(&mut self) {
        let mut new_clients = Vec::new();
        for tcp_sock in &self.tcp_sockets {
            // Accept a new TCP client.
            if let Some(client_sock) = self.tcpip.tcp_accept(&tcp_sock.sock) {
                let handle = self.next_sock_id;
                self.next_sock_id += 1;
                assert!(self.sockets.get(&handle).is_none());
                let new_tcp_sock = Rc::new(TcpSocket {
                    handle: handle,
                    sock: client_sock,
                    listener: tcp_sock.listener.clone(),
                });
                self.sockets.insert(handle, new_tcp_sock.clone());
                new_clients.push(new_tcp_sock);
                self.accepted_queue
                    .push((tcp_sock.listener.clone(), tcp_sock.handle, handle));
            }

            // Read received data.
            let readable_len = self.tcpip.tcp_readable_len(&tcp_sock.sock);
            if readable_len > 0 {
                use idl::memmgr::call_alloc_pages;
                let num_pages = align_up(readable_len, PAGE_SIZE) / PAGE_SIZE;
                let mut page = call_alloc_pages(&MEMMGR_SERVER, num_pages).unwrap();
                self.tcpip.tcp_read(&tcp_sock.sock, page.as_bytes_mut());
                self.received_queue
                    .push((tcp_sock.listener.clone(), tcp_sock.handle, page));
            }
        }

        // Send net_client.tcp_accpeted messages.
        let mut new_accepted_queue = Vec::new();
        for (listener, handle, client) in self.accepted_queue.drain(..) {
            match nbsend_tcp_accepted(&listener, handle, client) {
                Ok(_) => {}
                Err(Error::WouldBlock) | Err(Error::NeedsRetry) => {
                    new_accepted_queue.push((listener, handle, client));
                }
                Err(err) => {
                    warn!("failed to send net_client.tcp_accepted: {}", err);
                }
            }
        }

        // Send net_client.tcp_received messages.
        let mut new_received_queue = Vec::new();
        for (listener, handle, page) in self.received_queue.drain(..) {
            // FIXME: Avoid this (conditionally) needless memory copy.
            let cloned_page = page.clone();
            match nbsend_tcp_received(&listener, handle, page) {
                Ok(_) => {}
                Err(Error::WouldBlock) | Err(Error::NeedsRetry) => {
                    new_received_queue.push((listener, handle, cloned_page));
                }
                Err(err) => {
                    warn!("failed to send net_client.tcp_received: {}", err);
                }
            }
        }

        self.accepted_queue = new_accepted_queue;
        self.received_queue = new_received_queue;
        self.tcp_sockets.extend(new_clients);

        // Build and enqueue packets.
        oops_on_error!(self.tcpip.interval_work(self.uptime()));

        // Transmit packets.
        while let Some(mbuf) = self.tcpip.pop_tx_packet() {
            let frame = mbuf.as_bytes();
            let num_pages = align_up(frame.len(), PAGE_SIZE) / PAGE_SIZE;
            let mut page = idl::memmgr::call_alloc_pages(&MEMMGR_SERVER, num_pages).unwrap();
            page.copy_from_slice(frame);
            oops_on_error!(call_transmit(&self.network_device, page));
        }
    }

    pub fn add_tcp_sock(&mut self, sock: SocketHandle, listener: Rc<Channel>) -> Option<HandleId> {
        // FIXME:
        let handle = self.next_sock_id;
        self.next_sock_id += 1;
        assert!(self.sockets.get(&handle).is_none());
        let tcp_sock = Rc::new(TcpSocket {
            handle: handle,
            sock,
            listener,
        });

        self.sockets.insert(handle, tcp_sock.clone());
        self.tcp_sockets.push(tcp_sock);
        Some(handle)
    }
}

impl resea::idl::net::Server for Server {
    fn tcp_listen(&mut self, _from: &Channel, ch: Channel, port: u16) -> Result<HandleId> {
        let sock = self.tcpip.tcp_listen(Port::new(port));
        let handle = match self.add_tcp_sock(sock, Rc::new(ch)) {
            Some(handle) => handle,
            None => return Err(Error::NeedsRetry),
        };

        Ok(handle)
    }

    fn tcp_close(&mut self, _from: &Channel, handle: HandleId) -> Result<()> {
        let sock = match self.sockets.get(&handle) {
            Some(sock) => sock,
            None => return Err(Error::NotFound),
        };

        self.tcpip.tcp_close(&sock.sock);
        // TODO: Free resources.
        Ok(())
    }

    fn tcp_write(&mut self, _from: &Channel, handle: HandleId, data: Page) -> Result<()> {
        let sock = match self.sockets.get(&handle) {
            Some(sock) => sock,
            None => return Err(Error::NotFound),
        };

        // FIXME: Propagate the error.
        self.tcpip.tcp_write(&sock.sock, data.as_bytes()).unwrap();
        Ok(())
    }
}

impl resea::idl::network_device_client::Server for Server {
    fn received(&mut self, _from: &Channel, packet: Page) {
        self.tcpip.receive("net0", packet.as_bytes());
    }
}

impl idl::server::Server for Server {
    fn connect(
        &mut self,
        _from: &Channel,
        interface: InterfaceId,
    ) -> Result<(InterfaceId, Channel)> {
        assert!(interface == idl::net::INTERFACE_ID);
        let client_ch = Channel::create()?;
        client_ch.transfer_to(&self.ch)?;
        Ok((interface, client_ch))
    }
}

impl resea::mainloop::Mainloop for Server {
    fn deferred_work(&mut self) -> DeferredWorkResult {
        self.flush_queued_messages();
        DeferredWorkResult::Done
    }
}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let server_ch = Channel::create().unwrap();

    let listener_ch = Channel::create().unwrap();
    listener_ch.transfer_to(&server_ch).unwrap();
    let network_device = connect_to_server(idl::network_device::INTERFACE_ID)
        .expect("failed to connect to a network_device");
    idl::network_device::call_listen(&network_device, listener_ch).unwrap();

    let mut server = Server::new(server_ch, network_device);
    // Run flush_queued_messages() once to submit DHCP packets.
    server.flush_queued_messages();
    publish_server(idl::net::INTERFACE_ID, &server.ch).unwrap();

    info!("ready");
    mainloop!(&mut server, [server, net, network_device_client], []);
}
