use crate::channel::Channel;

#[derive(Debug, Copy, Clone)]
#[repr(transparent)]
pub struct Header(usize);

impl Header {
    pub fn from_usize(value: usize) -> Header {
        Header(value)
    }

    pub fn as_usize(self) -> usize {
        self.0
    }

    #[inline]
    pub fn interface_id(self) -> u8 {
        ((self.0 >> 8) & 0xff) as u8
    }

    #[inline]
    pub fn method_id(self) -> u8 {
        (self.0 & 0xff) as u8
    }

    #[inline]
    pub fn msg_id(self) -> u16 {
        (self.0 & 0xffff) as u16
    }
}

#[derive(Debug)]
pub enum PagePayload {
    Move {
        addr: usize,
        exp: usize,
    },
    Shared {
        addr: usize,
        exp: usize,
    },
}

#[derive(Debug)]
#[repr(transparent)]
pub struct Payload(usize);

impl Into<Payload> for usize {
    fn into(self)-> Payload {
        Payload(self)
    }
}

impl From<Payload> for usize {
    fn from(p: Payload) -> usize {
        payload_as_usize(p)
    }
}

impl Into<Payload> for isize {
    fn into(self)-> Payload {
        Payload(self as usize)
    }
}

impl From<Payload> for isize {
    fn from(p: Payload) -> isize {
        payload_as_usize(p) as isize
    }
}

impl Into<Payload> for u8 {
    fn into(self)-> Payload {
        Payload(self as usize)
    }
}

impl From<Payload> for u8 {
    fn from(p: Payload) -> u8 {
        payload_as_usize(p) as u8
    }
}

pub fn payload_as_usize(p: Payload) -> usize {
    p.0
}

fn usize_to_page_size(size: usize) -> usize {
    if size == 4096 {
        1
    } else {
        unimplemented!()
    }
}

pub struct Page(PagePayload);

impl Page {
    pub fn from_payload(p: PagePayload) -> Page {
        Page(p)
    }

    pub fn new(ptr: *const u8, size: usize) -> Page {
        // return an error
        assert!(ptr as usize % 4096 == 0);

        Page(PagePayload::Move {
            addr: ptr as usize,
            exp: usize_to_page_size(size),
        })
    }
}

impl Into<Payload> for Page {
    fn into(self)-> Payload {
        match self.0 {
            PagePayload::Move { addr, exp } => Payload((addr & 0xffff_ffff_fff_f000) | (exp << 3)),
            PagePayload::Shared { .. } => unimplemented!(),
        }
    }
}

impl From<Payload> for Page {
    fn from(_p: Payload) -> Page {
        unimplemented!();
    }
}

#[derive(Debug)]
pub struct Msg {
    pub header: Header,
    pub from: Channel,
    pub p0: Payload,
    pub p1: Payload,
    pub p2: Payload,
    pub p3: Payload,
    pub p4: Payload,
}
