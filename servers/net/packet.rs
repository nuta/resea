use resea::mem::size_of;

pub struct Packet<'a> {
    data: &'a [u8],
    offset: usize,
}

impl<'a> Packet<'a> {
    pub const fn new(data: &'a [u8]) -> Packet<'a> {
        Packet { data, offset: 0 }
    }

    pub fn consume<T>(&mut self) -> Option<&T> {
        let consume_len = size_of::<T>();
        let remaining_len = self.data.len() - self.offset;
        if consume_len > remaining_len {
            // The packet is too short.
            return None;
        }

        let offset = self.offset;
        self.offset += consume_len;
        unsafe {
            let reference = self.data.as_ptr().add(offset) as *const T;
            Some(&*reference)
        }
    }

    pub fn consume_bytes(&mut self, len: usize) -> Option<&[u8]> {
        if self.offset + len > self.data.len() {
            return None;
        }

        let offset = self.offset;
        self.offset += len;
        Some(&self.data[offset..(offset + len)])
    }
}
