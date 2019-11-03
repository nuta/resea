#![no_std]
#![feature(global_asm)]
#![feature(lang_items)]
#![feature(core_panic_info)]
#![feature(asm)]

#[macro_use]
extern crate resea;

mod arch;
mod main;
mod page;
