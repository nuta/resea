use crate::syscalls::{Result, SyscallError};
use crate::message::{Msg, Header, decode_payload};
use crate::channel::{Channel, CId};

const FLAG_SEND: usize = 1 << 26;
const FLAG_RECV: usize = 1 << 27;
const SYSCALL_IPC: u64 = 0;
const SYSCALL_OPEN: u64 = 2;
//const SYSCALL_CLOSE: u64 = 0;
const SYSCALL_LINK: u64 = 4;
const SYSCALL_TRANSFER: u64 = 5;

/// Converts a system call return code into a `Result`.
fn error_code_to_err<T, F: FnOnce() -> T>(code: i64, op: F) -> Result<T> {
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

pub fn call(ch: isize, pre_header: usize, p0: usize, p1: usize, p2: usize, p3: usize, p4: usize, accept: usize) -> Result<Msg> {
    unsafe {
        let error: i64;
        let r_header: u64;
        let r0: u64;
        let r1: u64;
        let r2: u64;
        let r3: u64;
        let r4: u64;

        let header = (ch as usize) << 48 | FLAG_SEND | FLAG_RECV | pre_header | accept << 32;
        asm!(
            "syscall"
            :  "={rax}"(error), "={rdi}"(r_header), "={rsi}"(r0), "={rdx}"(r1),
              "={r10}"(r2), "={r8}"(r3), "={r9}"(r4)
            : "{rax}"(SYSCALL_IPC), "{rdi}"(header), "{rsi}"(p0), "{rdx}"(p1),
              "{r10}"(p2), "{r8}"(p3), "{r9}"(p4)
            : "rcx", "rbx", "rbp", "r11", "r12", "r13", "r14", "r15"
        );

        error_code_to_err(error, ||{
            let from = r_header >> 48;
            Msg {
                from: Channel::from_cid(CId::new(from as isize)),
                header: Header::from_usize(header as usize),
                p0: decode_payload(r0 as usize),
                p1: decode_payload(r1 as usize),
                p2: decode_payload(r2 as usize),
                p3: decode_payload(r3 as usize),
                p4: decode_payload(r4 as usize),
            }
        })
    }
}

pub fn send(ch: isize, pre_header: usize, p0: usize, p1: usize, p2: usize, p3: usize, p4: usize) -> Result<()> {
    unsafe {
        let error: i64;
        let _r_header: u64;
        let _r0: u64;
        let _r1: u64;
        let _r2: u64;
        let _r3: u64;
        let _r4: u64;

        let header = ((ch as usize) << 48)| FLAG_SEND | pre_header;
        asm!(
            "syscall"
            : "={rax}"(error), "={rdi}"(_r_header), "={rsi}"(_r0), "={rdx}"(_r1),
              "={r10}"(_r2), "={r8}"(_r3), "={r9}"(_r4)
            : "{rax}"(SYSCALL_IPC), "{rdi}"(header), "{rsi}"(p0), "{rdx}"(p1),
              "{r10}"(p2), "{r8}"(p3), "{r9}"(p4)
            : "rcx", "rbx", "rbp", "r11", "r12", "r13", "r14", "r15"
        );

        error_code_to_err(error, || ())
    }
}

pub fn recv(ch: isize, accept: usize) -> Result<Msg> {
    unsafe {
        let error: i64;
        let r_header: u64;
        let r0: u64;
        let r1: u64;
        let r2: u64;
        let r3: u64;
        let r4: u64;

        let header = (ch as usize) << 48 | accept << 32 | FLAG_RECV;
        asm!(
            "syscall; xchg %bx, %bx; nop"
            :  "={rax}"(error), "={rdi}"(r_header), "={rsi}"(r0), "={rdx}"(r1),
              "={r10}"(r2), "={r8}"(r3), "={r9}"(r4)
            : "{rax}"(SYSCALL_IPC), "{rdi}"(header), "{rsi}"(0), "{rdx}"(0),
              "{r10}"(0), "{r8}"(0), "{r9}"(0)
            : "rcx", "rbx", "rbp", "r11", "r12", "r13", "r14", "r15"
        );

        error_code_to_err(error, || {
            let from = r_header >> 48;
            Msg {
                from: Channel::from_cid(CId::new(from as isize)),
                header: Header::from_usize(r_header as usize),
                p0: decode_payload(r0 as usize),
                p1: decode_payload(r1 as usize),
                p2: decode_payload(r2 as usize),
                p3: decode_payload(r3 as usize),
                p4: decode_payload(r4 as usize),
            }
        })
    }
}

pub fn open() -> Result<CId> {
    unsafe {
        let error: i64;
        let ch: u64;
        let _r0: u64;
        let _r1: u64;
        let _r2: u64;
        let _r3: u64;
        let _r4: u64;

        asm!(
            "syscall"
            :  "={rax}"(error), "={rax}"(ch), "={rsi}"(_r0), "={rdx}"(_r1),
              "={r10}"(_r2), "={r8}"(_r3), "={r9}"(_r4)
            : "{rax}"(SYSCALL_OPEN), "{rdi}"(0), "{rsi}"(0), "{rdx}"(0),
              "{r10}"(0), "{r8}"(0), "{r9}"(0)
            : "rcx", "rbx", "rbp", "r11", "r12", "r13", "r14", "r15"
        );


        error_code_to_err(error, || CId::new(ch as isize))
    }
}

pub fn link(ch1: CId, ch2: CId) -> Result<()> {
    unsafe {
        let error: i64;
        let _r0: u64;
        let _r1: u64;
        let _r2: u64;
        let _r3: u64;
        let _r4: u64;

        asm!(
            "syscall"
            : "={rax}"(error), "={rsi}"(_r0), "={rdx}"(_r1),
              "={r10}"(_r2), "={r8}"(_r3), "={r9}"(_r4)
            : "{rax}"(SYSCALL_LINK), "{rdi}"(ch1.as_isize()), "{rsi}"(ch2.as_isize()), "{rdx}"(0),
              "{r10}"(0), "{r8}"(0), "{r9}"(0)
            : "rcx", "rbx", "rbp", "r11", "r12", "r13", "r14", "r15"
        );

        error_code_to_err(error, || ())
    }
}

pub fn transfer(src: CId, dst: CId) -> Result<()> {
    unsafe {
        let error: i64;
        let _r0: u64;
        let _r1: u64;
        let _r2: u64;
        let _r3: u64;
        let _r4: u64;

        asm!(
            "syscall"
            : "={rax}"(error), "={rsi}"(_r0), "={rdx}"(_r1),
              "={r10}"(_r2), "={r8}"(_r3), "={r9}"(_r4)
            : "{rax}"(SYSCALL_TRANSFER), "{rdi}"(src.as_isize()), "{rsi}"(dst.as_isize()), "{rdx}"(0),
              "{r10}"(0), "{r8}"(0), "{r9}"(0)
            : "rcx", "rbx", "rbp", "r11", "r12", "r13", "r14", "r15"
        );

        error_code_to_err(error, || ())
    }
}