use resea::idl;
use resea::server::{ServerResult, publish_server};
use resea::result::Error;
use resea::channel::Channel;
use resea::message::{Page, Notification};
use resea::PAGE_SIZE;
use resea::utils::align_up;
use crate::e1000::Device;
use crate::pci::Pci;

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
    fn listen(&mut self, _from: &Channel, ch: Channel) -> ServerResult<()> {
        assert!(self.listener.is_none());
        self.listener = Some(ch);
        ServerResult::Ok(())
    }
    
    fn transmit(&mut self, _from: &Channel, packet: Page, len: usize) -> ServerResult<()> {
        let data = packet.as_bytes();
        if len > data.len() {
            return ServerResult::Err(Error::InvalidArg);
        }

        self.device.send_ethernet_packet(&data[..len]);
        ServerResult::Ok(())
    }
}

impl resea::server::Server for Server {
    fn notification(&mut self, _notification: Notification) {
        self.device.handle_interrupt();
        while let Some(pkt) = self.device.receive_ethernet_packet() {
            if let Some(ref listener) = self.listener {
                use idl::memmgr::Client;
                let num_pages = align_up(pkt.len(), PAGE_SIZE) / PAGE_SIZE;
                let mut page = MEMMGR_SERVER.alloc_pages(num_pages).unwrap();
                (&mut page.as_bytes_mut()[..pkt.len()]).copy_from_slice(&pkt);
                idl::network_device::send_received(listener, page, pkt.len()).ok();
            }
        }
    }
}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let mut server = Server::new();

    publish_server(idl::network_device::INTERFACE_ID, &server.ch).unwrap();
    serve_forever!(&mut server, [network_device]);
}
