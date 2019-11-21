#![no_std]
#![feature(asm)]
#![feature(global_asm)]
#![feature(lang_items)]
#![feature(core_panic_info)]
#![feature(alloc_error_handler)]
#![allow(clippy::missing_safety_doc)]

#[macro_use]
extern crate alloc;
pub use alloc::{format, vec};

/// A (partial) standard Rust library (`libstd`) for Resea apps.
pub mod std {
    // Prevent rustfmt from removing `::`.
    #[rustfmt::skip]
    pub use ::alloc::{alloc, borrow, boxed, fmt, rc, slice, str, string, vec};

    pub use core::{
        array, cell, char, clone, cmp, convert, default, hash, i16, i32, i64, i8, isize, marker,
        mem, num, ops, pin, ptr, result, u16, u32, u64, u8, usize,
    };
    pub mod sync {
        pub use alloc::sync::{Arc, Weak};
        pub use core::sync::atomic;
    }
}

#[macro_use]
pub mod print;

pub mod allocator;
pub mod backtrace;
pub mod channel;
pub mod collections;
pub mod idl;
pub mod lazy_static;
pub mod message;
pub mod result;
pub mod server;
pub mod thread_info;
pub mod utils;

mod arch;
mod init;

#[cfg(target_os = "resea")]
mod lang_items;

pub use arch::{breakpoint, PAGE_SIZE};

#[cfg(target_os = "resea")]
pub fn program_name() -> &'static str {
    env!("PROGRAM_NAME")
}

#[cfg(not(target_os = "resea"))]
pub fn program_name() -> &'static str {
    "(test program)"
}
