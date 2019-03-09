pub fn io_read8(port: usize) -> u8 {
    let value;
    unsafe {
        asm!("inb %dx, %al" : "={al}"(value) : "{dx}"(port) :: "volatile");
    }
    value
}

pub fn io_read16(port: usize) -> u16 {
    let value;
    unsafe {
        asm!("inw %dx, %ax" : "={ax}"(value) : "{dx}"(port) :: "volatile");
    }
    value
}

pub fn io_read32(port: usize) -> u32 {
    let value;
    unsafe {
        asm!("inl %dx, %eax" : "={eax}"(value) : "{dx}"(port) :: "volatile");
    }
    value
}

pub fn io_write8(port: usize, value: u8) {
    unsafe {
        asm!("outb %al, %dx" :: "{al}"(value), "{dx}"(port) :: "volatile");
    }
}

pub fn io_write16(port: usize, value: u16) {
    unsafe {
        asm!("outw %ax, %dx" :: "{ax}"(value), "{dx}"(port) :: "volatile");
    }
}

pub fn io_write32(port: usize, value: u32) {
    unsafe {
        asm!("outl %eax, %dx" :: "{eax}"(value), "{dx}"(port) :: "volatile");
    }
}
