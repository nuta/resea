use resea::result::Error;
use resea::idl;
use resea::PAGE_SIZE;
use resea::message::Page;
use resea::channel::Channel;
use crate::fat::FileSystem;

static MEMMGR_SERVER: Channel = Channel::from_cid(1);
static _KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    fs: FileSystem,
}

impl Server {
    pub fn new(fs: FileSystem) -> Server {
        Server {
            ch: Channel::create().unwrap(),
            fs,
        }
    }
}

// impl idl::fs::Server for Server {
// }

impl idl::server::Server for Server {
    fn connect(&mut self, _from: &Channel, interface_id: u8) -> Option<Result<Channel, Error>> {
        assert_eq!(interface_id, idl::fs::INTERFACE_ID);
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

    use idl::discovery::Client;
    let storage_device =
        MEMMGR_SERVER.connect(idl::storage_device::INTERFACE_ID)
            .expect("failed to connect to a storage_device server");

    let fs = FileSystem::new(storage_device, 0)
        .expect("failed to load the file system");

    let mut server = Server::new(fs);

    // let ch = Channel::create().unwrap();
    // ch.transfer_to(&server.ch).unwrap();
    // idl::discovery::Client::register(&MEMMGR_SERVER, idl::fs::INTERFACE_ID, ch).unwrap();

    serve_forever!(&mut server, [server]);
}
