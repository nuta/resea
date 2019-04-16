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
        ((self.msg_id() >> 8) & 0xff) as u8
    }

    #[inline]
    pub fn method_id(self) -> u8 {
        (self.msg_id() & 0xff) as u8
    }

    #[inline]
    pub fn msg_id(self) -> u16 {
        ((self.0 >> 16) & 0xffff) as u16
    }
}

fn usize_to_page_size(size: usize) -> usize {
    if size == 4096 {
        1
    } else {
        unimplemented!()
    }
}

#[repr(transparent)]
pub struct Page(usize);

impl Page {
    pub fn new(ptr: *const u8, size: usize) -> Page {
        let addr = ptr as usize;
        let exp = usize_to_page_size(size);
        // FIXME: return an error
        assert!(addr % 4096 == 0);
        Page((addr & 0xffff_ffff_fff_f000) | (exp << 3))
    }
}

pub trait Msg {}

const FIXED_STRING_LEN_MAX: usize = 128;

#[repr(C, packed)]
pub struct FixedString {
    len: u8,
    data: [u8; FIXED_STRING_LEN_MAX],
}

impl FixedString {
    pub fn from_str(str: &str) -> Option<FixedString> {
        if str.len() > FIXED_STRING_LEN_MAX {
            return None;
        }

        // TODO: copy the string without zero clear.
        let mut data = [0; FIXED_STRING_LEN_MAX];
        data.clone_from_slice(str.as_bytes());

        Some(FixedString {
            len: str.len() as u8,
            data,
        })
    }

    pub fn as_str(&self) -> Result<&str, core::str::Utf8Error> {
        core::str::from_utf8(&self.data[..self.len as usize])
    }
}
