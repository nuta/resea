use resea::channel::Channel;
use resea::message::*;

    use core::fmt::Write;

pub struct Printer();

impl Printer {
    pub const fn new() -> Printer {
        Printer()
    }
}

impl Write for Printer {
    fn  write_char(&mut self, c: char) -> core::fmt::Result {
        // Assuming that @1 is connected with a server which provides runtime
        // interface.
        let client = Channel::from_cid(1);
        use resea::idl::runtime::Client;
        client.printchar(c as u8).ok();
        Ok(())
    }

    fn  write_str(&mut self, s: &str) -> core::fmt::Result {
        for ch in s.chars() {
            self.write_char(ch).ok();
        }

        Ok(())
    }
}

#[no_mangle]
pub fn main() {
    info!("hello from memmgr!");
}
