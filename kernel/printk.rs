use core::panic;
use core::sync::atomic::{AtomicBool, Ordering};
use crate::arch;
use crate::stats;
use crate::debug::backtrace;
use core::convert::TryInto;

pub fn print_str(str: &str) {
    for ch in str.chars() {
        arch::printchar(ch);
    }
}

pub fn print_unsigned_int<T: TryInto<usize>>(value: T, base: usize, pad_ch: char, pad_len: usize) {
    value
        .try_into()
        .map(|mut value: usize| {
            const MAX_LEN: usize = 32;
            let mut buf = ['0'; MAX_LEN];
            let mut start = MAX_LEN - 1;

            if value == 0 {
                // `buf[start]` is initialized by '0'.
                start -= 1;
            }

            while value > 0 {
                let digit = value % base;
                let ch = match digit {
                    0x0 => '0',
                    0x1 => '1',
                    0x2 => '2',
                    0x3 => '3',
                    0x4 => '4',
                    0x5 => '5',
                    0x6 => '6',
                    0x7 => '7',
                    0x8 => '8',
                    0x9 => '9',
                    0xa => 'a',
                    0xb => 'b',
                    0xc => 'c',
                    0xd => 'd',
                    0xe => 'e',
                    0xf => 'f',
                    _ => unreachable!(),
                };

                buf[start] = ch;
                value /= base;
                start -= 1;
            }

            for _ in (MAX_LEN - start - 1)..pad_len {
                arch::printchar(pad_ch);
            }

            for ch in buf.iter().skip(start + 1) {
                arch::printchar(*ch);
            }
        })
        .ok();
}

pub fn print_signed_int<T: TryInto<isize>>(value: T, base: usize, pad_ch: char, pad_len: usize) {
    value
        .try_into()
        .map(|value: isize| {
            if value < 0 {
                arch::printchar('-');
                print_unsigned_int(value.abs(), base, pad_ch, pad_len);
            }

            print_unsigned_int(value.abs(), base, pad_ch, pad_len);
        })
        .ok();
}

pub enum Argument<'a> {
    Str(&'a str),
    Signed(isize),
    Unsigned(usize),
}

pub trait Displayable<'a> {
    fn display(&self) -> Argument<'a>;
}

impl<'a> Displayable<'a> for i32 {
    fn display(&self) -> Argument<'a> {
        Argument::Signed(*self as isize)
    }
}

impl<'a> Displayable<'a> for isize {
    fn display(&self) -> Argument<'a> {
        Argument::Signed(*self as isize)
    }
}

impl<'a> Displayable<'a> for u8 {
    fn display(&self) -> Argument<'a> {
        Argument::Unsigned(*self as usize)
    }
}

impl<'a> Displayable<'a> for u16 {
    fn display(&self) -> Argument<'a> {
        Argument::Unsigned(*self as usize)
    }
}

impl<'a> Displayable<'a> for u32 {
    fn display(&self) -> Argument<'a> {
        Argument::Unsigned(*self as usize)
    }
}

impl<'a> Displayable<'a> for u64 {
    fn display(&self) -> Argument<'a> {
        Argument::Unsigned(*self as usize)
    }
}

impl<'a> Displayable<'a> for usize {
    fn display(&self) -> Argument<'a> {
        Argument::Unsigned(*self as usize)
    }
}

impl<'a> Displayable<'a> for &'a str {
    fn display(&self) -> Argument<'a> {
        Argument::Str(self)
    }
}

fn print_arg<'a>(fmt: &Argument<'a>, spec: char) {
    let (base, pad_char, pad_len) = match spec {
        'd' => (10, '0', 0),
        'x' => (16, '0', 0),
        'p' => (16, '0', core::mem::size_of::<usize>() * 2),
        'v' | _ => (10, ' ', 0),
    };

    match *fmt {
        Argument::Str(value) => print_str(value),
        Argument::Signed(value) => {
            print_signed_int(value, base, pad_char, pad_len);
        }
        Argument::Unsigned(value) => {
            print_unsigned_int(value, base, pad_char, pad_len);
        }
    }
}

pub fn do_printk<'a>(fmt: &'static str, args: &[Argument<'a>]) {
    let mut fmt_iter = fmt.chars().peekable();
    let mut args_iter = args.iter();
    loop {
        match (fmt_iter.next(), fmt_iter.peek()) {
            (Some('%'), Some('%')) => arch::printchar('%'),
            (Some('%'), Some(spec)) => {
                let arg = args_iter.next().expect("too few arguments");
                print_arg(arg, *spec);

                // Skip `spec`.
                fmt_iter.next();
            }
            (Some(ch), _) => arch::printchar(ch),
            (None, _) => break,
        }
    }
}

/// Prints a string and a newline.
#[macro_export]
macro_rules! printk {
    ($fmt:expr) => { crate::printk::do_printk(concat!($fmt, "\n"), &[]); };
    ($fmt:expr, $($arg:expr), *) => {{
        use crate::printk::Displayable;
        crate::printk::do_printk(concat!($fmt, "\n"), &[$($arg.display()),*]);
    }};
}

/// Prints a trace message.
#[macro_export]
macro_rules! trace {
    ($fmt:expr) => { crate::printk::do_printk(concat!($fmt, "\n"), &[]); };
    ($fmt:expr, $($arg:expr), *) => {{
        use crate::printk::Displayable;
        crate::printk::do_printk(concat!($fmt, "\n"), &[$($arg.display()),*]);
    }};
}

/// Prints a trace message with user context for debugging.
#[macro_export]
macro_rules! trace_user {
    ($fmt:expr) => {{
        use crate::printk::Displayable;
        crate::printk::do_printk(
            concat!("#%v.%v: ", $fmt, "\n"),
            &[
                crate::arch::current().process().pid().display(),
                crate::arch::current().tid().display()
            ]
        );
    }};
    ($fmt:expr, $($arg:expr), *) => {{
        use crate::printk::Displayable;
        crate::printk::do_printk(
            concat!("#%v.%v: ", $fmt, "\n"),
            &[
                crate::arch::current().process().pid().display(),
                crate::arch::current().tid().display(),
                $($arg.display()),*
            ]
        );
    }};
}

/// Prints a string and backtrace. Useful for debugging and for repoting
/// an unintended condition which are not critical.
#[macro_export]
macro_rules! oops {
    () => {
        crate::printk::do_printk(concat!("[Oops] (", file!(), ":", line!(), ")\n"), &[]);
        crate::debug::backtrace();
    };
    ($fmt:expr) => {
        crate::printk::do_printk(concat!("[Oops] ", $fmt, " (", file!(), ":", line!(), ")\n"), &[]);
        crate::debug::backtrace();
    };
    ($fmt:expr, $($arg:expr), *) => {{
        use crate::printk::Displayable;
        crate::printk::do_printk(concat!("[Oops] ", $fmt, " (", file!(), ":", line!(), ")\n"), &[$($arg.display()),*]);
        crate::debug::backtrace();
    }}
}

macro_rules! core_fmt_printk {
    ($($arg:tt)*) => {{
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

        #[allow(unused_import)]
        use core::fmt::Write;
        write!(Logger::new(), "{}\n", format_args!($($arg)*)).ok();
    }};
}

static ALREADY_PANICED: AtomicBool = AtomicBool::new(false);

#[panic_handler]
#[no_mangle]
#[cfg(not(test))]
pub fn panic(info: &panic::PanicInfo) -> ! {
    if ALREADY_PANICED.load(Ordering::SeqCst) {
        // Prevent recursive panic since symbols::backtrace() may panic.
        print_str("[DOUBLE PANIC] panicked in the panic handler\n");
        arch::halt();
    } else {
        ALREADY_PANICED.store(true, Ordering::SeqCst);
        do_panic(info);
    }
}

#[cfg(not(test))]
fn do_panic(info: &panic::PanicInfo) -> ! {
    core_fmt_printk!("[PANIC] {}", info);
    backtrace();
    core_fmt_printk!("");
    stats::print();
    core_fmt_printk!("");
    crate::arch::halt();
}
