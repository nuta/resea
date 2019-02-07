use crate::syscalls;
use crate::message::{Msg, Accept, encode_payload};

#[derive(Copy, Clone, Debug)]
#[repr(transparent)]
pub struct CId(isize);

impl CId {
    pub fn new(cid: isize) -> CId {
        CId(cid)
    }

    pub fn as_isize(&self) -> isize {
        self.0
    }
}

#[derive(Copy, Clone, Debug)]
pub struct Channel {
    cid: CId,
}

impl Channel {
    pub fn from_cid(cid: CId) -> Channel {
        Channel {
            cid
        }
    }

    pub fn create() -> syscalls::Result<(Channel)> {
        syscalls::open().map(Channel::from_cid)
    }

    pub fn link(&self, ch: &Channel) -> syscalls::Result<()> {
        syscalls::link(self.cid, ch.cid)
    }

    pub fn transfer_to(&self, dest: &Channel) -> syscalls::Result<()> {
        syscalls::transfer(self.cid, dest.cid)
    }

    pub fn send(&self, m: Msg) -> syscalls::Result<()> {
        syscalls::send(
            self.cid.as_isize(),
            m.header.as_usize(),
            encode_payload(m.p0),
            encode_payload(m.p1),
            encode_payload(m.p2),
            encode_payload(m.p3),
            encode_payload(m.p4),
        )
    }

    pub fn recv(&self) -> syscalls::Result<Msg> {
        let accept = Accept::allocate();
        syscalls::recv(self.cid.as_isize(), accept.as_usize())
    }

    pub fn call(&self, m: Msg) -> syscalls::Result<Msg> {
        let accept = Accept::allocate();
        syscalls::call(
            self.cid.as_isize(),
            m.header.as_usize(),
            encode_payload(m.p0),
            encode_payload(m.p1),
            encode_payload(m.p2),
            encode_payload(m.p3),
            encode_payload(m.p4),
            accept.as_usize()
        )
    }
}