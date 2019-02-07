use core::fmt::Write;
use crate::channel::{CId, Channel};

pub struct Printer();

impl Printer {
    pub const fn new() -> Printer {
        Printer()
    }
}

impl Write for Printer {
    fn  write_char(&mut self, c: char) -> core::fmt::Result {
        let putchar = crate::idl::putchar::Client::from_channel(Channel::from_cid(CId::new(1)));
        putchar.putchar(c as usize).ok();
        Ok(())
    }

    fn  write_str(&mut self, s: &str) -> core::fmt::Result {
        for ch in s.chars() {
            self.write_char(ch).ok();
        }

        Ok(())
    }
}

/// Prints a string.
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
        #[allow(unused_import)]
        use core::fmt::Write;
        write!(::resea::print::Printer::new(), "{}", format_args!($($arg)*)).ok();
    }};
}

/// Prints a string and a newline.
#[macro_export]
macro_rules! println {
    ($fmt:expr) => { ::resea::print!(concat!($fmt, "\n")); };
    ($fmt:expr, $($arg:tt)*) => { ::resea::print!(concat!($fmt, "\n"), $($arg)*); };
}

/// Prints a string and a newline.
#[macro_export]
macro_rules! internal_print {
    ($($arg:tt)*) => {{
        #[allow(unused_import)]
        use core::fmt::Write;
        write!(crate::print::Printer::new(), "{}", format_args!($($arg)*)).ok();
    }};
}

/// Prints a string and a newline.
#[macro_export]
macro_rules! internal_println {
    ($fmt:expr) => { crate::internal_print!(concat!($fmt, "\n")); };
    ($fmt:expr, $($arg:tt)*) => { crate::internal_print!(concat!($fmt, "\n"), $($arg)*); };
}