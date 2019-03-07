use crate::syscalls::{Result, SyscallError};
use crate::channel::CId;

const FLAG_SEND: u64 = 1 << 0;
const FLAG_RECV: u64 = 1 << 1;
const SYSCALL_IPC: u64 = 0;
const SYSCALL_OPEN: u64 = 2;
//const SYSCALL_CLOSE: u64 = 0;
const SYSCALL_LINK: u64 = 4;
const SYSCALL_TRANSFER: u64 = 5;

/// Converts a system call return code into a `Result`.
fn syscall_error_into_result<T, F: FnOnce() -> T>(code: i64, op: F) -> Result<T> {
    if code < 0 {
        match code {
            -1 => Err(SyscallError::InvalidSyscall),
            -2 => Err(SyscallError::InvalidChannelId),
            -3 => Err(SyscallError::AlreadyReceived),
            -4 => Err(SyscallError::InvalidPayload),
            _ => Err(SyscallError::Unknown),
        }
    } else {
        Ok(op())
    }
}

unsafe fn syscall(syscall_type: u64, a0: u64, a1: u64) -> i64 {
    let retvalue: i64;
    asm!(
        "syscall"
        : "={rax}"(retvalue)
        : "{rax}"(syscall_type), "{rdi}"(a0), "{rsi}"(a1)
        : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11",
        "rbx", "rbp", "r12", "r13", "r14", "r15"
    );

    return retvalue;
}

pub unsafe fn update_thread_local_buffer<T>(m: &T) {
    let len = core::mem::size_of::<T>();
    asm!(r#"
        mov %fs:(0), %rdi
        cld
        rep movsb
    "#
        :: "{rsi}"(m as *const _ as u64), "{rcx}"(len)
        : "rdi"
    );
}

pub unsafe fn read_thread_local_buffer<T>() -> T {
    let len = core::mem::size_of::<T>();
    let mut m = core::mem::uninitialized();

    asm!(r#"
        mov %fs:(0), %rsi
        cld
        rep movsb
    "#
        :: "{rdi}"(&mut m as *mut _ as u64), "{rcx}"(len)
        : "rsi"
    );

    m
}

pub fn call(ch: CId) -> Result<()> {
    unsafe {
        let options = FLAG_SEND | FLAG_RECV;
        let error = syscall(SYSCALL_IPC, ch.as_isize() as u64, options);
        syscall_error_into_result(error, || ())
    }
}

pub fn send(ch: CId) -> Result<()> {
    unsafe {
        let error = syscall(SYSCALL_IPC, ch.as_isize() as u64, FLAG_SEND);
        syscall_error_into_result(error, || ())
    }
}


pub fn recv(ch: CId) -> Result<()> {
    unsafe {
        let error = syscall(SYSCALL_IPC, ch.as_isize() as u64, FLAG_RECV);
        syscall_error_into_result(error, || ())
    }
}

pub fn open() -> Result<CId> {
    unsafe {
        let rax = syscall(SYSCALL_OPEN, 0, 0);
        syscall_error_into_result(rax, || CId::new(rax as isize))
    }
}

pub fn link(ch1: CId, ch2: CId) -> Result<()> {
    unsafe {
        let error = syscall(SYSCALL_LINK, ch1.as_isize() as u64, ch2.as_isize() as u64);
        syscall_error_into_result(error, || ())
    }
}

pub fn transfer(src: CId, dst: CId) -> Result<()> {
    unsafe {
        let error = syscall(SYSCALL_TRANSFER, src.as_isize() as u64, dst.as_isize() as u64);
        syscall_error_into_result(error, || ())
    }
}
