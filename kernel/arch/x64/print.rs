use super::serial;
use super::vga;

pub fn printchar(ch: char) {
    unsafe {
        serial::putchar(ch);
        vga::putchar(ch);
    }
}
