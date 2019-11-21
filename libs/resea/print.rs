use core::fmt::Write;
use crate::channel::Channel;

pub struct Printer();

impl Printer {
    pub const fn new() -> Printer {
        Printer()
    }
}

impl Write for Printer {
    fn  write_char(&mut self, c: char) -> core::fmt::Result {
        printchar(c as u8);
        Ok(())
    }

    fn  write_str(&mut self, s: &str) -> core::fmt::Result {
        print_str(s);
        Ok(())
    }
}

pub fn printchar(ch: u8) {
    // Assuming that @1 is connected with a server which provides runtime
    // interface.
    let client = Channel::from_cid(1);
    use crate::idl::runtime::Client;
    client.printchar(ch).ok();
}

pub fn print_str(s: &str) {
    use crate::std::borrow::ToOwned;

    // Assuming that @1 is connected with a server which provides runtime
    // interface.
    let client = Channel::from_cid(1);
    use crate::idl::runtime::Client;
    client.print_str(s.to_owned()).ok();
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
        $crate::println!(concat!("\x1b[0;94m", "[{}] ", $fmt, "\x1b[0m"), $crate::program_name());
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[0;94m", "[{}] ", $fmt, "\x1b[0m"), $crate::program_name(), $($arg)*);
    };
}

/// Prints a warning message.
#[macro_export]
macro_rules! warn {
    ($fmt:expr) => {
        $crate::println!(concat!("\x1b[0;33m", "[{}] WARN: ", $fmt, "\x1b[0m"), $crate::program_name());
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[0;33m", "[{}] WARN: ", $fmt, "\x1b[0m"), $crate::program_name(), $($arg)*);
    };
}

/// Prints a trace message.
#[macro_export]
macro_rules! trace {
    ($fmt:expr) => {
        $crate::println!(concat!("[{}] ", $fmt), $crate::program_name());
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("[{}] ", $fmt), $crate::program_name(), $($arg)*);
    };
}

/// Prints a warning message with backtrace.
#[macro_export]
macro_rules! oops {
    ($fmt:expr) => {
        $crate::println!(concat!("\x1b[0;33m", "[{}] Oops: ", $fmt, "\x1b[0m"), $crate::program_name());
        $crate::backtrace::backtrace();
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[0;33m", "[{}] Oops: ", $fmt, "\x1b[0m"), $crate::program_name(), $($arg)*);
        $crate::backtrace::backtrace();
    };
}

/// Prints an error message.
#[macro_export]
macro_rules! error {
    ($fmt:expr) => {
        $crate::println!(concat!("\x1b[1;91m", "[{}] Error: ", $fmt, "\x1b[0m"), $crate::program_name());
        $crate::backtrace::backtrace();
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[1;91m", "[{}] Error: ", $fmt, "\x1b[0m"), $crate::program_name(), $($arg)*);
        $crate::backtrace::backtrace();
    };
}
