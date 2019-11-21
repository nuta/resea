#![no_std]
#![feature(global_asm)]

#[macro_use]
extern crate resea;

mod arch;
mod main;
mod initfs;
mod process;
mod elf;
