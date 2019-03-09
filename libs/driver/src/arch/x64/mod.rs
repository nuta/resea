mod ioport;

pub use self::ioport::*;

use resea::idl::kernel::AllowIoResponseMsg;
use resea::idl;

// Connected by memmgr.
const KERNEL_CID: isize = 2;

pub fn allow_io() -> resea::syscalls::Result<AllowIoResponseMsg> {
    let kernel = idl::kernel::Client::from_raw_cid(KERNEL_CID);
    kernel.allow_io()
}
