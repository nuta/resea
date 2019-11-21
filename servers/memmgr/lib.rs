#![no_std]
#![feature(global_asm)]

#[macro_use]
extern crate resea;

mod arch;
mod elf;
mod initfs;
mod main;
mod process;
