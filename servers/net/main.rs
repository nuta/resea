use resea::idl;
use resea::idl::network_device::{call_get_macaddr, call_transmit};
use resea::prelude::*;
use resea::server::{connect_to_server, DeferredWorkResult};
use resea::utils::align_up;
use resea::PAGE_SIZE;
use tcpip::{DeviceIpAddr, Instance, MacAddr, Port, SocketHandle};

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
        let test_sock = tcpip.tcp_listen(Port::new(80));
        let ma = call_get_macaddr(&network_device).unwrap();
        let macaddr = MacAddr::new([ma.0, ma.1, ma.2, ma.3, ma.4, ma.5]);
        tcpip.add_ethernet_device("net0", macaddr, DeviceIpAddr::Dhcp);
        tcpip.interval_work().unwrap(); // TODO: remove
        Server {
            ch: server_ch,
            network_device,
            tcpip,
            test_sock,
            clients: Vec::new(),
        }
    }

    pub fn flush(&mut self) {
        while let Some(mbuf) = self.tcpip.pop_tx_packet() {
            let frame = mbuf.as_bytes();
            let num_pages = align_up(frame.len(), PAGE_SIZE) / PAGE_SIZE;
            let mut page = idl::memmgr::call_alloc_pages(&MEMMGR_SERVER, num_pages).unwrap();
            page.copy_from_slice(frame);
            call_transmit(&self.network_device, page).unwrap();
        }
    }
}

impl resea::server::Server for Server {
    fn deferred_work(&mut self) -> DeferredWorkResult {
        loop {
            let mut new_resp = false;
            self.tcpip.interval_work().unwrap();
            self.flush();

            if let Some(client) = self.tcpip.tcp_accept(&self.test_sock) {
                info!("new client!");
                self.clients.push((String::new(), client));
            }

            for (req, sock) in &mut self.clients {
                info!("reading from sock...");
                let mut buf = Vec::new();
                self.tcpip.tcp_read(sock, &mut buf, 1024);
                req.push_str(resea::str::from_utf8(&buf).unwrap_or(""));

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

        DeferredWorkResult::Done
    }

    // FIXME:
    #[allow(safe_packed_borrows)]
    fn unknown_message(&mut self, m: &mut Message) -> bool {
        info!("received a message...");
        if m.header == idl::network_device::RECEIVED_MSG {
            let m = unsafe {
                resea::mem::transmute::<&mut Message, &mut idl::network_device::ReceivedMsg>(m)
            };

            self.tcpip
                .receive("net0", &m.packet.as_bytes()[..m.packet.len()]);
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

    let network_device = connect_to_server(idl::network_device::INTERFACE_ID)
        .expect("failed to connect to a network_device");

    idl::network_device::call_listen(&network_device, listener_ch).unwrap();

    let mut server = Server::new(server_ch, network_device);
    server.flush();

    info!("ready");
    resea::thread_info::alloc_and_set_page_base();
    serve_forever!(&mut server, []);
}
