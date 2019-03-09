#![no_std]
#![feature(alloc)]

#[macro_use]
extern crate resea;
extern crate alloc;

use resea::Channel;

struct VirtioNetServer {
}

impl VirtioNetServer {
    pub fn new() -> VirtioNetServer {
        VirtioNetServer {
        }
    }
}

fn main() {
    println!("virtio-net: starting");
    let mut server = VirtioNetServer::new();
    let mut server_ch = Channel::create();

    // serve_forever!(&mut server, &mut server_ch, [putchar]);
}
