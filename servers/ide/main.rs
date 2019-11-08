use resea::error::Error;
use resea::idl;
use resea::idl::storage_device::INTERFACE_ID;
use resea::PAGE_SIZE;
use resea::message::Page;
use resea::channel::Channel;
use crate::ide::IdeDevice;

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
    fn read(&mut self, _from: &Channel, sector: usize, num_sectors: usize) -> Option<Result<Page, Error>> {
        use idl::memmgr::Client;
        let num_pages = (num_sectors * self.device.sector_size()) / PAGE_SIZE;
        let mut page = MEMMGR_SERVER.alloc_pages(num_pages).unwrap();
        self.device.read_sectors(sector, num_sectors, page.as_bytes_mut()).unwrap();
        Some(Ok(page))
    }
}

impl idl::server::Server for Server {
    fn connect(&mut self, _from: &Channel, interface_id: u8) -> Option<Result<Channel, Error>> {
        assert_eq!(interface_id, INTERFACE_ID);
        let ch = Channel::create().unwrap();
        ch.transfer_to(&self.ch).unwrap();
        Some(Ok(ch))
    }
}

impl resea::server::Server for Server {
}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let mut server = Server::new();

    let ch = Channel::create().unwrap();
    ch.transfer_to(&server.ch).unwrap();
    idl::discovery::Client::register(&MEMMGR_SERVER, INTERFACE_ID, ch).unwrap();

    serve_forever!(&mut server, [storage_device, server]);
}
