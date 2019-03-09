#![no_std]
#![feature(asm)]
#![feature(alloc)]

extern crate alloc;
extern crate resea;

pub mod arch;
pub mod io_space;

pub use io_space::{IoSpace, IoSpaceType};
