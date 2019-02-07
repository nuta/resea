#![no_std]
#![feature(asm)]
#![feature(global_asm)]
#![feature(lang_items)]
#![feature(panic_info_message)]
#![feature(concat_idents)]
#![feature(integer_atomics)]
#![feature(const_fn)]
#![feature(core_panic_info)]
#![feature(const_raw_ptr_deref)]
#![feature(const_raw_ptr_to_usize_cast)]
#![feature(try_from)]
#![feature(core_intrinsics)]

#[cfg(test)]
#[macro_use]
extern crate std;

#[macro_use]
mod printk;
#[cfg_attr(test, allow(unused))]
mod debug;
#[macro_use]
mod utils;
#[macro_use]
mod collections;
mod arch;
mod process;
mod channel;
mod thread;
mod ipc;
mod server;
mod memory;
mod timer;
mod boot;
mod initfs;
mod allocator;
mod stats;
