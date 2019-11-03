#![cfg_attr(not(test), no_std)]
#![feature(asm)]
#![feature(global_asm)]
#![feature(lang_items)]
#![feature(core_panic_info)]
#![feature(alloc_error_handler)]

#![allow(unused)]

#[macro_use]
extern crate alloc;
pub use alloc::{vec, format};

/// A (partial) standard Rust library (`libstd`) for Resea apps.
pub mod std {
    pub use core::{
        u8, u16, u32, u64, i8, i16, i32, i64, char, isize, usize,
        result, mem, ptr, array, cell, hash, marker, cmp, ops, convert, default,
        clone, pin,
    };
    pub use ::alloc::{
        alloc, boxed, borrow, collections, fmt, rc, slice, str, string,
        sync, vec
    };
}


#[macro_use]
pub mod print;

pub mod error;
pub mod backtrace;
pub mod channel;
pub mod server;
pub mod idl;
pub mod message;
pub mod utils;

mod arch;
mod allocator;
mod init;
mod lang_items;

pub use arch::PAGE_SIZE;
