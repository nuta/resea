use crate::ide::IdeDevice;
use resea::channel::Channel;
use resea::idl;
use resea::idl::storage_device::INTERFACE_ID;
use resea::message::{InterfaceId, Page};
use resea::result::Error;
use resea::server::{publish_server, ServerResult};
use resea::utils::align_up;
use resea::PAGE_SIZE;

static MEMMGR_SERVER: Channel = Channel::from_cid(1);
static KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    device: IdeDevice,
}

impl Server {
    pub fn new() -> Server {
        Server {
            ch: Channel::create().unwrap(),
            device: IdeDevice::new(&KERNEL_SERVER),
        }
    }
}

impl idl::storage_device::Server for Server {
    fn read(&mut self, _from: &Channel, sector: usize, num_sectors: usize) -> ServerResult<Page> {
        if num_sectors == 0 {
            return ServerResult::Err(Error::InvalidArg);
        }

        use idl::memmgr::Client;
        let num_pages = align_up(num_sectors * self.device.sector_size(), PAGE_SIZE) / PAGE_SIZE;
        let mut page = MEMMGR_SERVER.alloc_pages(num_pages).unwrap();
        self.device
            .read_sectors(sector, num_sectors, page.as_bytes_mut())
            .unwrap();
        ServerResult::Ok(page)
    }
}

impl idl::server::Server for Server {
    fn connect(
        &mut self,
        _from: &Channel,
        interface: InterfaceId,
    ) -> ServerResult<(InterfaceId, Channel)> {
        assert!(interface == idl::storage_device::INTERFACE_ID);
        let client_ch = Channel::create().unwrap();
        client_ch.transfer_to(&self.ch).unwrap();
        ServerResult::Ok((interface, client_ch))
    }
}

impl resea::server::Server for Server {}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let mut server = Server::new();

    publish_server(INTERFACE_ID, &server.ch).unwrap();
    serve_forever!(&mut server, [storage_device, server]);
}
