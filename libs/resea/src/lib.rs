#![no_std]
#![feature(asm)]
#![feature(global_asm)]
#![feature(lang_items)]
#![feature(alloc)]
#![feature(panic_info_message)]
#![feature(concat_idents)]
#![feature(const_fn)]
#![feature(core_panic_info)]
#![feature(const_raw_ptr_deref)]
#![feature(const_raw_ptr_to_usize_cast)]
#![feature(core_intrinsics)]

extern crate alloc;

#[macro_use]
pub mod print;
#[macro_use]
pub mod server;
#[cfg(not(std))]
mod langitems;
mod allocator;
mod arch;
pub mod channel;
pub mod syscalls;
pub mod message;
pub mod idl;

pub use channel::Channel;

#[cfg(not(std))]
#[global_allocator]
static GLOBAL_ALLOCATOR: allocator::MyAllocator = allocator::MyAllocator {};
