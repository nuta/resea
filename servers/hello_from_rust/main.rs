#![no_std]
#![feature(asm)]
#![feature(global_asm)]
#![feature(llvm_asm)] // FIXME: Use newer inline assembly.
#![feature(lang_items)]
#![feature(alloc_error_handler)]
#![allow(clippy::missing_safety_doc)]

use core::alloc::Layout;
use core::panic;

#[lang = "eh_personality"]
#[no_mangle]
#[cfg(not(test))]
pub fn eh_personality() {
    loop {} // FIXME:
}

#[panic_handler]
#[no_mangle]
#[cfg(not(test))]
pub fn panic(_info: &panic::PanicInfo) -> ! {
    loop {} // FIXME:
}

#[alloc_error_handler]
#[cfg(not(test))]
fn alloc_error(_layout: Layout) -> ! {
    loop {} // FIXME:
}

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

#[allow(non_camel_case_types)]
pub type c_int = i32;
#[allow(non_camel_case_types)]
pub type c_long = i64;

pub unsafe fn syscall(
    n: c_int,
    a1: c_long,
    a2: c_long,
    a3: c_long,
    a4: c_long,
    a5: c_long,
) -> c_long {
    let ret: c_long;
    llvm_asm!(
        "syscall"
        : "={rax}"(ret)
        : "{rdi}"(n), "{rsi}"(a1), "{rdx}"(a2), "{r10}"(a3),
          "{r8}"(a4), "{r9}"(a5)
        : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
        : "volatile"
    );

    ret
}

pub unsafe fn sys_print(s: *const u8, len: usize) {
    syscall(6, s as c_long, len as c_long, 0, 0, 0);
}

pub fn printchar(ch: u8) {
    let s = [ch; 1];
    unsafe {
        sys_print(s.as_ptr(), 1);
    }
}

pub fn print_str(s: &str) {
    unsafe {
        sys_print(s.as_ptr(), s.len());
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
        write!($crate::Printer::new(), "{}", format_args!($($arg)*)).ok();
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

#[no_mangle]
pub fn main() {
    info!("Hello, World from Rust!");
}
