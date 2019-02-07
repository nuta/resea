use core::panic;
use core::sync::atomic::{AtomicBool, Ordering};
use core::fmt::Write;
use crate::arch;
use crate::stats;
use crate::debug::backtrace;

pub struct Logger();

impl Logger {
    pub const fn new() -> Logger {
        Logger()
    }
}

impl Write for Logger {
    fn  write_char(&mut self, c: char) -> core::fmt::Result {
        crate::arch::printchar(c);
        Ok(())
    }

    fn  write_str(&mut self, s: &str) -> core::fmt::Result {
        for ch in s.chars() {
            arch::printchar(ch);
        }

        Ok(())
    }
}

/// Prints a string.
/// TODO: Abandon core::fmt because it consumes too much memory especially stack.
#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => {{
        #[allow(unused_import)]
        use core::fmt::Write;
        write!(crate::printk::Logger::new(), "{}", format_args!($($arg)*)).ok();
    }};
}

/// Prints a string and a newline.
#[macro_export]
macro_rules! printk {
    ($fmt:expr) => { crate::print!(concat!($fmt, "\n")); };
    ($fmt:expr, $($arg:tt)*) => { crate::print!(concat!($fmt, "\n"), $($arg)*); };
}

/// Print error message and panic.
#[macro_export]
macro_rules! bug {
    ($fmt:expr) => { panic!(concat!("BUG: ", $fmt)); };
    ($fmt:expr, $($arg:tt)*) => { panic!(concat!("BUG: ", $fmt), $($arg)*); };
}

/// Prints a trace message for debugging.
#[macro_export]
macro_rules! trace {
    ($fmt:expr) => { crate::print!(concat!($fmt, "\n")); };
    ($fmt:expr, $($arg:tt)*) => { crate::print!(concat!($fmt, "\n"), $($arg)*); };
}

/// Prints a trace message with user context for debugging.
#[macro_export]
macro_rules! trace_user {
    ($fmt:expr) => {
        crate::print!(
            concat!("#{}.{}: ", $fmt, "\n"),
            crate::arch::current().process().pid().as_isize(),
            crate::arch::current().tid().as_isize()
        );
    };
    ($fmt:expr, $($arg:tt)*) => {
        crate::print!(
            concat!("#{}.{}: ", $fmt, "\n"),
            crate::arch::current().process().pid().as_isize(),
            crate::arch::current().tid().as_isize(),
            $($arg)*
        );
    };
}

/// Prints a string and backtrace. Useful for debugging and for repoting
/// an unintended condition which are not critical.
#[macro_export]
macro_rules! oops {
    () => {
        crate::print!(concat!("[Oops] (", file!(), ":", line!(), ")"));
        crate::debug::backtrace();
    };
    ($fmt:expr) => {
        crate::print!(concat!("[Oops] ", $fmt, " (", file!(), ":", line!(), ")"));
        crate::debug::backtrace();
    };
    ($fmt:expr, $($arg:tt)*) => {
        crate::print!(concat!("[Oops] ", $fmt, " (", file!(), ":", line!(), ")"), $($arg)*);
        crate::debug::backtrace();
    }
}

static ALREADY_PANICED: AtomicBool = AtomicBool::new(false);

#[panic_handler]
#[no_mangle]
#[cfg(not(test))]
pub fn panic(info: &panic::PanicInfo) -> ! {
    if ALREADY_PANICED.load(Ordering::SeqCst) {
        // Prevent recursive panic since symbols::backtrace() may panic.
        printk!("[DOUBLE PANIC] panicked in the panic handler: {}", info);
        arch::halt();
    } else {
        ALREADY_PANICED.store(true, Ordering::SeqCst);
        do_panic(info);
    }
}

#[cfg(not(test))]
fn do_panic(info: &panic::PanicInfo) -> ! {
    printk!("[PANIC] {}", info);
    backtrace();
    printk!("");
    stats::print();
    printk!("");
    crate::arch::halt();
}