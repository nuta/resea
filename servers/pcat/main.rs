use resea::channel::Channel;
use resea::idl;
use crate::pcat::Device;

static _MEMMGR_SERVER: Channel = Channel::from_cid(1);
static KERNEL_SERVER: Channel = Channel::from_cid(2);

struct Server {
    ch: Channel,
    device: Device,
}

impl Server {
    pub fn new() -> Server {
        Server {
            ch: Channel::create().unwrap(),
            device: Device::new(&KERNEL_SERVER),
        }
    }
}

impl resea::server::Server for Server {
}

#[no_mangle]
pub fn main() {
    info!("starting...");
    let mut server = Server::new();
    server.device.clear();
    server.device.print_str("Hello world from pcat driver!");
    serve_forever!(&mut server, []);
}
