use crate::{Error, Result};
use resea::cmp::min;
use resea::vec::Vec;

pub struct RingBuffer {
    buffer: Vec<u8>,
    capacity: usize,
    write_offset: usize,
    read_offset: usize,
}

impl RingBuffer {
    pub fn new(capacity: usize) -> RingBuffer {
        let mut buffer = vec![0; capacity];
        RingBuffer {
            buffer,
            capacity,
            write_offset: 0,
            read_offset: 0,
        }
    }

    pub fn write(&mut self, data: &[u8]) -> Result<()> {
        let len = data.len();
        if len > self.writable_len() {
            return Err(Error::BufferFull);
        }

        /*
        warn!("ringbuf[r={}, w={}, rl={}, wl={}]: write {}",
        self.read_offset, self.write_offset,
        self.readable_len(), self.writable_len(),
        len
        );
        */

        let end = min(self.write_offset + len, self.capacity);
        let copy_len = end - self.write_offset;
        (&mut self.buffer[self.write_offset..end]).copy_from_slice(&data[..copy_len]);

        let remaining_len = len - copy_len;
        if remaining_len > 0 {
            (&mut self.buffer[0..remaining_len]).copy_from_slice(&data[copy_len..]);
        }

        self.write_offset = (self.write_offset + len) % self.capacity;
        Ok(())
    }

    pub fn read(&mut self, len: usize, buf: &mut [u8]) {
        self.peek(len, buf);
        self.read_offset = (self.read_offset + len) % self.capacity;
    }

    pub fn peek(&mut self, len: usize, buf: &mut [u8]) {
        assert!(len <= self.readable_len());
        /*
        warn!("ringbuf[r={}, w={}, rl={}, wl={}]: read {}",
            self.read_offset, self.write_offset,
            self.readable_len(), self.writable_len(),
            len);
        */

        let end = min(self.read_offset + len, self.capacity);
        let copy_len = end - self.read_offset;
        (&mut buf[0..copy_len]).copy_from_slice(&self.buffer[self.read_offset..end]);

        let remaining_len = len - copy_len;
        if remaining_len > 0 {
            (&mut buf[copy_len..copy_len + remaining_len])
                .copy_from_slice(&self.buffer[0..remaining_len]);
        }
    }

    pub fn discard(&mut self, len: usize) {
        warn!(
            "ringbuf[r={}, w={}, rl={}, wl={}]: discard {}",
            self.read_offset,
            self.write_offset,
            self.readable_len(),
            self.writable_len(),
            len
        );
        self.read_offset = (self.read_offset + len) % self.capacity;
    }

    pub fn writable_len(&self) -> usize {
        if self.read_offset > self.write_offset {
            self.read_offset - self.write_offset
        } else {
            // Subtract by 1 so that write_offset won't be equal to
            // read_offset.
            self.read_offset + (self.capacity - self.write_offset - 1)
        }
    }

    pub fn readable_len(&self) -> usize {
        if self.write_offset >= self.read_offset {
            self.write_offset - self.read_offset
        } else {
            self.write_offset + (self.capacity - self.read_offset)
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test() {
        let mut buf = RingBuffer::new(4);
        assert_eq!(buf.writable_len(), 3);
        assert_eq!(buf.readable_len(), 0);

        assert_eq!(buf.write(b"ABC"), Ok(()));
        assert_eq!(buf.writable_len(), 0);
        assert_eq!(buf.readable_len(), 3);

        assert_eq!(buf.write(b"XYZ"), Err(Error::BufferFull));
        assert_eq!(buf.writable_len(), 0);
        assert_eq!(buf.readable_len(), 3);

        let mut read_buf = Vec::new();
        buf.peek(2, &mut read_buf);
        assert_eq!(&read_buf, b"AB");
        assert_eq!(buf.writable_len(), 0);
        assert_eq!(buf.readable_len(), 3);

        read_buf.clear();
        buf.read(3, &mut read_buf);
        assert_eq!(&read_buf, b"ABC");
        assert_eq!(buf.writable_len(), 3);
        assert_eq!(buf.readable_len(), 0);

        assert_eq!(buf.write(b"DEF"), Ok(()));
        read_buf.clear();
        buf.read(3, &mut read_buf);
        assert_eq!(&read_buf, b"DEF");
        assert_eq!(buf.writable_len(), 3);
        assert_eq!(buf.readable_len(), 0);
    }
}
