use crate::arch;

#[derive(Debug)]
pub enum IoSpaceType {
    IoMappedIo,
}

#[derive(Debug)]
pub struct IoSpace {
    space_type: IoSpaceType,
    base: usize,
    len: usize,
}

impl IoSpace {
    pub fn new(space_type: IoSpaceType, base: usize, len: usize) -> IoSpace {
        IoSpace {
            space_type,
            base,
            len,
        }
    }

    pub fn read8(&mut self, offset: usize) -> u8 {
        match self.space_type {
            IoSpaceType::IoMappedIo => {
                arch::io_read8(self.base + offset)
            }
        }
    }

    pub fn read16(&mut self, offset: usize) -> u16 {
        match self.space_type {
            IoSpaceType::IoMappedIo => {
                arch::io_read16(self.base + offset)
            }
        }
    }

    pub fn read32(&mut self, offset: usize) -> u32 {
        match self.space_type {
            IoSpaceType::IoMappedIo => {
                arch::io_read32(self.base + offset)
            }
        }
    }

    pub fn write8(&mut self, offset: usize, value: u8) {
        match self.space_type {
            IoSpaceType::IoMappedIo => {
                arch::io_write8(self.base + offset, value);
            }
        }
    }

    pub fn write16(&mut self, offset: usize, value: u16) {
        match self.space_type {
            IoSpaceType::IoMappedIo => {
                arch::io_write16(self.base + offset, value);
            }
        }
    }

    pub fn write32(&mut self, offset: usize, value: u32) {
        match self.space_type {
            IoSpaceType::IoMappedIo => {
                arch::io_write32(self.base + offset, value);
            }
        }
    }
}
