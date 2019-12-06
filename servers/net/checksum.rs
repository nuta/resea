use crate::endian::{swap16, swap32};
use crate::ip::IpAddr;
use resea::mem::size_of;
use resea::slice;

#[repr(transparent)]
#[derive(Clone, Copy, Debug)]
pub struct Checksum(u32);

impl Checksum {
    pub const fn new() -> Checksum {
        Checksum(0)
    }

    pub fn input_u16<T: Into<u16>>(&mut self, data: T) {
        self.0 += data.into() as u32;
    }

    pub fn input_u32<T: Into<u32>>(&mut self, data: T) {
        let value = data.into();
        self.input_u16((value & 0xffff) as u16);
        self.input_u16((value >> 16) as u16);
    }

    pub fn input_struct<T>(&mut self, data: &T) {
        let len = size_of::<T>();
        debug_assert!(len % 2 == 0);
        let words = unsafe { slice::from_raw_parts(data as *const T as *const u16, len / 2) };

        for word in words {
            self.0 += *word as u32;
        }
    }

    pub fn input_bytes(&mut self, bytes: &[u8]) {
        if bytes.len() == 0 {
            return;
        }

        let odd = bytes.len() % 2 != 0;
        let words: &[u16] =
            unsafe { slice::from_raw_parts(bytes.as_ptr() as *const u16, bytes.len() / 2) };

        // Sum up the input.
        for word in words {
            self.0 += *word as u32;
        }

        // Handle the last byte if the length of input is odd.
        if odd {
            self.0 += bytes[bytes.len() - 1] as u32;
        }
    }

    pub fn finish(self) -> u16 {
        let mut checksum = (self.0 >> 16) + (self.0 & 0xffff);
        checksum += checksum >> 16;
        swap16(!checksum as u16)
    }
}

pub fn compute_pseudo_header_checksum<T>(
    checksum: &mut Checksum,
    remote_addr: IpAddr,
    local_addr: IpAddr,
    proto: u8,
    header: &T,
    data_len: usize
) {
    // Pseudo header.
    match remote_addr {
        IpAddr::Ipv4(ipv4_addr) => checksum.input_u32(swap32(ipv4_addr.as_u32())),
    }
    match local_addr {
        IpAddr::Ipv4(ipv4_addr) => checksum.input_u32(swap32(ipv4_addr.as_u32())),
    }
    checksum.input_u16(swap16(proto as u16));
    checksum.input_u16(swap16((size_of::<T>() + data_len) as u16));

    // TCP header.
    checksum.input_struct(header);
}
