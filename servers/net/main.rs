use resea::collections::Vec;
use resea::std::string::String;
use resea::idl;
use resea::channel::Channel;
use resea::message::Message;
use tcpip::{Instance, IpAddr, Ipv4Addr, Ipv4Network, Port, SocketHandle, MacAddr};
use resea::PAGE_SIZE;
use resea::utils::align_up;

static MEMMGR_SERVER: Channel = Channel::from_cid(1);
static _KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    network_device: Channel,
    tcpip: Instance,
    clients: Vec<(String, SocketHandle)>,
    test_sock: SocketHandle,
}

impl Server {
    pub fn new(server_ch: Channel, network_device: Channel) -> Server {
        let mut tcpip = Instance::new();
        let test_sock =
            tcpip.tcp_listen(IpAddr::Ipv4(Ipv4Addr::new(10, 0, 2, 15)), Port::new(80));
        tcpip.add_ethernet_device("net0", MacAddr::new([0x52, 0x54, 0x0, 0x12, 0x34, 0x56]) /* TODO: */, Some(Ipv4Addr::new(10, 0, 2, 15)));
        tcpip.add_route("net0",
            Ipv4Network::new(10, 0, 2, 0, 0xffffff00),
            Ipv4Addr::new(10, 0, 2, 15)
    );

        Server {
            ch: server_ch,
            network_device,
            tcpip,
            test_sock,
            clients: Vec::new(),
        }
    }
}

impl resea::server::Server for Server {
    fn deferred_work(&mut self) {
        loop {
            let mut new_resp = false;
            self.tcpip.interval_work().unwrap();

            while let Some(mbuf) = self.tcpip.pop_tx_packet() {
                use resea::idl::network_device::Client;
                let frame = mbuf.as_bytes();
                let num_pages = align_up(frame.len(), PAGE_SIZE) / PAGE_SIZE;
                let mut page =
                    idl::memmgr::Client::alloc_pages(&MEMMGR_SERVER, num_pages).unwrap();
                (&mut page.as_bytes_mut()[..frame.len()]).copy_from_slice(frame);
                self.network_device.transmit(page, frame.len()).unwrap();
            }

            if let Some(client) = self.tcpip.tcp_accept(&self.test_sock) {
                info!("new client!");
                self.clients.push((String::new(), client));
            }

            for (req, sock) in &mut self.clients {
                info!("reading from sock...");
                let mut buf = Vec::new();
                self.tcpip.tcp_read(sock, &mut buf, 1024);
                req.push_str(resea::std::str::from_utf8(&buf).unwrap_or(""));

                if req.contains("\r\n\r\n") {
                    info!("received a HTTP request:");
                    info!("{}", req);
                    let resp =
    b"HTTP/1.1 200 OK\r\nServer: Resea TCP/IP Server\r\nConnection: close\r\nContent-Length: 31\r\n\r\n<html><h1>It works!</h1></html>";
                    self.tcpip.tcp_write(sock, resp);
                    self.tcpip.tcp_close(sock);
                    new_resp = true;
                    req.clear();
                    // TODO: Remove the client sock.
                }
            }

            if !new_resp {
                break;
            }
        }
    }

    // FIXME:
    #[allow(safe_packed_borrows)]
    fn unknown_message(&mut self, m: &mut Message) -> bool {
        info!("received a message...");
        if m.header == idl::network_device::RECEIVED_MSG {
            let m = unsafe {
                resea::std::mem::transmute::<&mut Message, &mut idl::network_device::ReceivedMsg>(m)
            };

            self.tcpip.receive("net0", &m.packet.as_bytes()[..m.len]);
            resea::thread_info::alloc_and_set_page_base();
        }

        false
    }
}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let server_ch = Channel::create().unwrap();
    let listener_ch = Channel::create().unwrap();
    listener_ch.transfer_to(&server_ch).unwrap();

    let network_device = idl::discovery::Client::connect(&MEMMGR_SERVER, idl::network_device::INTERFACE_ID)
        .expect("failed to connect to a network_device");
    
    idl::network_device::Client::listen(&network_device, listener_ch).unwrap();

    let mut server = Server::new(server_ch, network_device);

    info!("ready");
    resea::thread_info::alloc_and_set_page_base();
    serve_forever!(&mut server, []);
}
