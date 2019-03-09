#![no_std]
#![feature(alloc)]

#[macro_use]
extern crate resea;
extern crate resea_driver;
extern crate alloc;

mod pci;

use resea::Channel;

struct DrvMgrServer {
}

impl DrvMgrServer {
    pub fn new() -> DrvMgrServer {
        DrvMgrServer {
        }
    }
}

impl resea::idl::drvmgr::Server for DrvMgrServer {
}

fn main() {
    println!("drvmgr: starting");
    let mut server = DrvMgrServer::new();
    let mut server_ch = Channel::create().unwrap();

    resea_driver::arch::x64::allow_io().expect("failed to allow_io");
    println!("drvmgr: enumerating peripherals connected over PCI...");
    pci::scan_pci_bus();

    serve_forever!(&mut server, &mut server_ch, [drvmgr]);
}
