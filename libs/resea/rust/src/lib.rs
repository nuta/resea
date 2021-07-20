//! # The Resea standard library for Rust
//! This library provides Rusty APIs for Resea applications based on Rust's
//! powerful libraries such as libcore and liballoc.
//!
//! ```
//! #![no_std]
//!
//! use resea::{info, vec::Vec, collections::BTreeMap};
//!
//! #[no_mangle]
//! pub fn main() {
//!     info!("Hello World from Rust!");
//!     let mut v = Vec::new();
//!     v.push(7);
//!     v.push(8);
//!     v.push(9);
//!     info!("vec test: {:?}", v);
//!
//!     let mut m = BTreeMap::new();
//!     m.insert("a", "it works");
//!     info!("btreemap test: {:?}", m.get("a"));
//! }
//! ```
//!
//! ## Resea-specific Modules
//! - [`mod@print`]: Print functions.
//! - [`mod@capi`]: Resea C standard library APIs (e.g. system calls).
#![no_std]
#![feature(lang_items)]
#![feature(alloc_error_handler)]

extern crate alloc;

pub use alloc::*;
pub use core::*;

mod allocator;
pub mod capi;
mod lang_items;
pub mod print;
mod stub_helpers;

// Automatically generated.
pub mod stubs;
