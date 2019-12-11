//! # Resea Standard Library
//! 
//! Resea Standard Library provides essential features including:
//! 
//! - Language primitives and core features: [`RefCell<T>`], [`Rc<T>`], [`Option<T>`], etc.
//! - Collections: [`Vec<T>`], [`VecDeque<T>`], [`HashMap<K, V>`], etc.
//! - Resea-specific primitives: [`Channel`], [`Message`], [`Page`], etc.
//! - IPC stubs: [`resea::idl`]
//! - Useful macros: [`trace!`], [`info!`], [`oops!`], [`mainloop!`] etc.
//! 
//! ## Prelude
//! Use the prelude module to import frequently used stuff:
//! ```
//! use resea::prelude::*;
//! ```
//! 
//! ## Resea-specific Modules
//! - [`allocator`]
//! - [`backtrace`]
//! - [`channel`]
//! - [`collections`]
//! - [`idl`]
//! - [`lazy_static`]
//! - [`mainloop`]
//! - [`message`]
//! - [`page`]
//! - [`prelude`]
//! - [`result`]
//! - [`server`]
//! - [`thread_info`]
//! - [`utils`]
//! 
//! [`Vec<T>`]: collections/struct.Vec.html
//! [`VecDeque<T>`]: collections/struct.VecDeque.html
//! [`HashMap<K, V>`]: collections/struct.HashMap.html
//! [`RefCell<T>`]: cell/struct.RefCell.html
//! [`Rc<T>`]: rc/struct.Rc.html
//! [`Option<T>`]: option/enum.Option.html
//! [`Channel`]: channel/struct.Channel.html
//! [`Message`]: message/struct.Message.html
//! [`Page`]: page/struct.Page.html
//! [`resea::idl`]: resea/idl/index.html
//! [`allocator`]: resea/allocator/index.html
//! [`backtrace`]: resea/backtrace/index.html
//! [`channel`]: resea/channel/index.html
//! [`collections`]: resea/collections/index.html
//! [`idl`]: resea/idl/index.html
//! [`lazy_static`]: resea/lazy_static/index.html
//! [`mainloop`]: resea/mainloop/index.html
//! [`message`]: resea/message/index.html
//! [`page`]: resea/page/index.html
//! [`prelude`]: resea/prelude/index.html
//! [`result`]: resea/result/index.html
//! [`server`]: resea/server/index.html
//! [`thread_info`]: resea/thread_info/index.html
//! [`utils`]: resea/utils/index.html

//! 
//! 
//! 

#![no_std]
#![feature(asm)]
#![feature(global_asm)]
#![feature(lang_items)]
#![feature(core_panic_info)]
#![feature(alloc_error_handler)]
#![allow(clippy::missing_safety_doc)]

#[macro_use]
extern crate alloc as alloc_crate;

/// A (partial) standard Rust library (`libstd`) for Resea apps.
// Prevent rustfmt from removing `::`.
#[rustfmt::skip]
pub use alloc_crate::{
    alloc, borrow, boxed, fmt, rc, slice, str, string, format, vec
};

pub use core::{
    array, cell, char, clone, cmp, convert, default, hash, i16, i32, i64, i8, isize, marker, mem,
    option, num, ops, pin, ptr, u16, u32, u64, u8, usize,
};
pub mod sync {
    pub use alloc_crate::sync::{Arc, Weak};
    pub use core::sync::atomic;
}

#[macro_use]
pub mod print;

pub mod allocator;
pub mod backtrace;
pub mod channel;
pub mod collections;
pub mod idl;
pub mod lazy_static;
pub mod mainloop;
pub mod message;
pub mod page;
pub mod prelude;
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
    "(program name)"
}

#[cfg(target_os = "resea")]
pub fn version() -> &'static str {
    env!("VERSION")
}

#[cfg(not(target_os = "resea"))]
pub fn version() -> &'static str {
    "(version)"
}
