#[derive(Debug)]
pub enum SyscallError {
    /// Must be a bug in this library or kernel.
    Unknown,
}

pub type Result<T> = core::result::Result<T, SyscallError>;
pub use crate::arch::syscalls::{open, send, recv, call, link, transfer};
