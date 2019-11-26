#[repr(transparent)]
#[derive(Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Debug)]
pub struct NetEndian<T>(T);

impl<T: Into<NetEndian<T>>> NetEndian<T> {
    pub fn new(value: T) -> NetEndian<T> {
        value.into()
    }
}

impl Into<NetEndian<u16>> for u16 {
    fn into(self) -> NetEndian<u16> {
        NetEndian(swap16(self))
    }
}

impl Into<NetEndian<u32>> for u32 {
    fn into(self) -> NetEndian<u32> {
        NetEndian(swap32(self))
    }
}

impl Into<u16> for NetEndian<u16> {
    fn into(self) -> u16 {
        swap16(self.0)
    }
}

impl Into<u32> for NetEndian<u32> {
    fn into(self) -> u32 {
        swap32(self.0)
    }
}

pub fn swap16(x: u16) -> u16 {
    // TODO: Do nothing on big-endian CPUs.
    ((x & 0xff00) >> 8) | ((x & 0x00ff) << 8)
}

pub fn swap32(x: u32) -> u32 {
    // TODO: Do nothing on big-endian CPUs.
    ((x & 0xff000000) >> 24)
        | ((x & 0x00ff0000) >> 8)
        | ((x & 0x0000ff00) << 8)
        | ((x & 0x000000ff) << 24)
}
