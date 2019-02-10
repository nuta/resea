#[derive(Debug)]
pub enum SyscallError {
    Ok,
    /// Returned in the arch's system call handler.
    InvalidSyscall,
    /// Invalid channel ID including unused and already closed
    /// channels.
    InvalidChannelId,
    /// Failed to get the receiver right. Use a mutex in the when
    /// you share a channel by multiple threads!
    AlreadyReceived,
    /// Invalid payload.
    InvalidPayload,
    /// Must be a bug in this library or kernel.
    Unknown,
}

pub type Result<T> = core::result::Result<T, SyscallError>;
pub use crate::arch::syscalls::{open, send, recv, call, link, transfer};
