use crate::e1000::Device;
use crate::pci::Pci;
use resea::channel::Channel;
use resea::idl;
use resea::message::{InterfaceId, Notification, Page};
use resea::result::Error;
use resea::server::{publish_server, ServerResult};
use resea::utils::align_up;
use resea::PAGE_SIZE;

static MEMMGR_SERVER: Channel = Channel::from_cid(1);
static KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    listener: Option<Channel>,
    device: Device,
}

impl Server {
    pub fn new() -> Server {
        let pci = Pci::new(&KERNEL_SERVER);
        let ch = Channel::create().unwrap();
        let mut device = Device::new(&ch, &pci, &KERNEL_SERVER, &MEMMGR_SERVER);
        device.init();

        info!("initialized the device");
        info!("MAC address = {:x?}", device.mac_addr());

        Server {
            ch,
            listener: None,
            device,
        }
    }
}

impl idl::network_device::Server for Server {
    fn get_macaddr(&mut self, _from: &Channel) -> ServerResult<(u8, u8, u8, u8, u8, u8)> {
        let m = self.device.mac_addr();
        ServerResult::Ok((m[0], m[1], m[2], m[3], m[4], m[5]))
    }

    fn listen(&mut self, _from: &Channel, ch: Channel) -> ServerResult<()> {
        assert!(self.listener.is_none());
        self.listener = Some(ch);
        ServerResult::Ok(())
    }

    fn transmit(&mut self, _from: &Channel, packet: Page) -> ServerResult<()> {
        let data = packet.as_bytes();
        if packet.len() > data.len() {
            return ServerResult::Err(Error::InvalidArg);
        }

        self.device.send_ethernet_packet(data);
        ServerResult::Ok(())
    }
}

impl idl::server::Server for Server {
    fn connect(
        &mut self,
        _from: &Channel,
        interface: InterfaceId,
    ) -> ServerResult<(InterfaceId, Channel)> {
        assert!(interface == idl::network_device::INTERFACE_ID);
        let client_ch = Channel::create().unwrap();
        client_ch.transfer_to(&self.ch).unwrap();
        ServerResult::Ok((interface, client_ch))
    }
}

impl resea::server::Server for Server {
    fn notification(&mut self, _notification: Notification) {
        self.device.handle_interrupt();
        let rx_queue = self.device.rx_queue();
        while let Some(pkt) = rx_queue.front() {
            if let Some(ref listener) = self.listener {
                use idl::memmgr::Client;
                let num_pages = align_up(pkt.len(), PAGE_SIZE) / PAGE_SIZE;
                let mut page = MEMMGR_SERVER.alloc_pages(num_pages).unwrap();
                page.copy_from_slice(&pkt);
                let reply = idl::network_device::nbsend_received(listener, page);
                match reply {
                    // Try later.
                    Err(Error::NeedsRetry) => (),
                    _ => {
                        rx_queue.pop_front();
                    }
                }
            } else {
                // No listeners. Drop the received packets.
                rx_queue.pop_front();
            }
        }
    }
}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let mut server = Server::new();

    publish_server(idl::network_device::INTERFACE_ID, &server.ch).unwrap();
    serve_forever!(&mut server, [server, network_device]);
}
