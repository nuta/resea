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
        // Assuming that @1 is connected with a server which provides runtime
        // interface.
        let client = Channel::from_cid(1);
        use crate::idl::runtime::Client;
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

/// Prints a string.
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
        #[allow(unused_import)]
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
        $crate::println!(concat!("\x1b[0;94m", "[{}] ", $fmt, "\x1b[0m"), env!("PROGRAM_NAME"));
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[0;94m", "[{}] ", $fmt, "\x1b[0m"), env!("PROGRAM_NAME"), $($arg)*);
    };
}

/// Prints a warning message.
#[macro_export]
macro_rules! warn {
    ($fmt:expr) => {
        $crate::println!(concat!("\x1b[0;33m", "[{}] WARN: ", $fmt, "\x1b[0m"), env!("PROGRAM_NAME"));
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[0;33m", "[{}] WARN: ", $fmt, "\x1b[0m"), env!("PROGRAM_NAME"), $($arg)*);
    };
}

/// Prints a warning message with backtrace.
#[macro_export]
macro_rules! oops {
    ($fmt:expr) => {
        $crate::println!(concat!("\x1b[0;33m", "[{}] Oops: ", $fmt, "\x1b[0m"), env!("PROGRAM_NAME"));
        $crate::backtrace::backtrace();
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[0;33m", "[{}] Oops: ", $fmt, "\x1b[0m"), env!("PROGRAM_NAME"), $($arg)*);
        $crate::backtrace::backtrace();
    };
}

/// Prints an error message.
#[macro_export]
macro_rules! error {
    ($fmt:expr) => {
        $crate::println!(concat!("\x1b[1;91m", "[{}] Error: ", $fmt, "\x1b[0m"), env!("PROGRAM_NAME"));
        $crate::backtrace::backtrace();
    };
    ($fmt:expr, $($arg:tt)*) => {
        $crate::println!(concat!("\x1b[1;91m", "[{}] Error: ", $fmt, "\x1b[0m"), env!("PROGRAM_NAME"), $($arg)*);
        $crate::backtrace::backtrace();
    };
}
