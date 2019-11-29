use crate::ide::IdeDevice;
use resea::idl;
use resea::idl::storage_device::INTERFACE_ID;
use resea::prelude::*;
use resea::server::{publish_server};
use resea::utils::align_up;
use resea::PAGE_SIZE;

static MEMMGR_SERVER: Channel = Channel::from_cid(1);
static KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    device: IdeDevice,
}

impl Server {
    pub fn new(device: IdeDevice) -> Server {
        Server {
            ch: Channel::create().unwrap(),
            device,
        }
    }
}

impl idl::storage_device::Server for Server {
    fn read(&mut self, _from: &Channel, sector: usize, num_sectors: usize) -> Result<Page> {
        if num_sectors == 0 {
            return Err(Error::InvalidArg);
        }

        use idl::memmgr::call_alloc_pages;
        let num_pages = align_up(num_sectors * self.device.sector_size(), PAGE_SIZE) / PAGE_SIZE;
        let mut page = call_alloc_pages(&MEMMGR_SERVER, num_pages)?;
        self.device.read_sectors(sector, num_sectors, page.as_bytes_mut())?;
        Ok(page)
    }
}

impl idl::server::Server for Server {
    fn connect(
        &mut self,
        _from: &Channel,
        interface: InterfaceId,
    ) -> Result<(InterfaceId, Channel)> {
        assert!(interface == idl::storage_device::INTERFACE_ID);
        let client_ch = Channel::create()?;
        client_ch.transfer_to(&self.ch)?;
        Ok((interface, client_ch))
    }
}

impl resea::server::Server for Server {}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let device = IdeDevice::new(&KERNEL_SERVER);
    let mut server = Server::new(device);

    publish_server(INTERFACE_ID, &server.ch).unwrap();
    serve_forever!(&mut server, [storage_device, server]);
}
