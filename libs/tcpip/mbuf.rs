use resea::std::mem::size_of;
use resea::std::slice;
use resea::collections::Vec;

pub struct Mbuf {
    buffer: Vec<u8>,
}

impl Mbuf {
    pub fn new() -> Mbuf {
        Mbuf {
            buffer: Vec::new(),
        }
    }

    pub fn as_bytes(&self) -> &[u8] {
        &self.buffer
    }

    pub fn len(&self) -> usize {
        self.buffer.len()
    }

    pub fn append_bytes(&mut self, data: &[u8]) {
        self.buffer.extend_from_slice(data);
    }

    pub fn append<T>(&mut self, data: &T) {
        let data_len = size_of::<T>();
        self.append_bytes(unsafe {
            slice::from_raw_parts(data as *const T as *const u8, data_len)
        });
    }

    pub fn prepend<T>(&mut self, data: &T) {
        let old_buffer = self.buffer.clone();
        let data_len = size_of::<T>();
        self.buffer.clear();
        self.buffer.reserve(data_len + self.buffer.len());
        self.buffer.extend_from_slice(unsafe {
            slice::from_raw_parts(data as *const T as *const u8, data_len)
        });
        self.buffer.extend_from_slice(old_buffer.as_slice());
    }
}
