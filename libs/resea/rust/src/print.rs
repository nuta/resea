use crate::capi::{size_t, sys_console_write};

pub struct Printer();

impl Printer {
    pub const fn new() -> Printer {
        Printer()
    }
}

impl core::fmt::Write for Printer {
    fn write_char(&mut self, c: char) -> core::fmt::Result {
        printchar(c as u8);
        Ok(())
    }

    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        print_str(s);
        Ok(())
    }
}

pub fn printchar(ch: u8) {
    let s = [ch; 1];
    unsafe {
        sys_console_write(s.as_ptr(), 1);
    }
}

pub fn print_str(s: &str) {
    unsafe {
        sys_console_write(s.as_ptr(), s.len() as size_t);
    }
}

pub fn program_name() -> &'static str {
    env!("PROGRAM_NAME")
}

/// Prints a string.
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
        #![allow(unused_import)]
        use core::fmt::Write;
        write!($crate::print::Printer::new(), "{}", format_args!($($arg)*)).ok();
    }};
}

/// Prints a string and a newline.
#[macro_export]
macro_rules! println {
    ($fmt:expr) => { $crate::print!(concat!($fmt, "\n")); };
    ($fmt:expr, $($arg:tt)*) => { $crate::print!(concat!($fmt, "\n"), $($arg)*); };
}

/// Prints a warning message.
#[macro_export]
macro_rules! info {
    ($fmt:expr) => {
        $crate::println!(concat!("\x1b[0;94m", "[{}] ", $fmt, "\x1b[0m"), $crate::print::program_name());
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[0;94m", "[{}] ", $fmt, "\x1b[0m"), $crate::print::program_name(), $($arg)*);
    };
}
