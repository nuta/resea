use crate::result::{Error, Result};

const SYSCALL_IPC: u32 = 0;
const SYSCALL_OPEN: u32 = 1;
const SYSCALL_TRANSFER: u32 = 4;
const IPC_SEND: u32 = 1 << 8;
const IPC_RECV: u32 = 1 << 9;
const IPC_NOBLOCK: u32 = 1 << 10;

unsafe fn convert_error(error: i32) -> Result<()> {
    if error < 0 {
        Err(core::mem::transmute::<u8, Error>(-error as u8))
    } else {
        Ok(())
    }
}

unsafe fn ipc_syscall(cid: i32, ops: u32) -> Result<()> {
    let error: i32;
    asm!(
        "syscall"
        : "={eax}"(error)
        : "{eax}"(SYSCALL_IPC | ops),
          "{rdi}"(cid)
        : "memory", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
        : "volatile"
    );

    convert_error(error)
}

pub unsafe fn nop() {
    let error: i32;
    asm!(
        "nop"
        : "={eax}"(error)
        : "{eax}"(6),
          "{rdi}"(0xdead)
        : // "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
        : "volatile"
    );
}

pub unsafe fn open() -> Result<i32> {
    let cid_or_error: i32;
    asm!(
        "syscall"
        : "={eax}"(cid_or_error)
        : "{eax}"(SYSCALL_OPEN)
        : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
        : "volatile"
    );

    if cid_or_error < 0 {
        Err(core::mem::transmute::<u8, Error>(-cid_or_error as u8))
    } else {
        Ok(cid_or_error)
    }
}

pub unsafe fn transfer(src: i32, dst: i32) -> Result<()> {
    let error: i32;
    asm!(
        "syscall"
        : "={eax}"(error)
        : "{eax}"(SYSCALL_TRANSFER),
          "{rdi}"(src),
          "{rsi}"(dst)
        : "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
        : "volatile"
    );

    convert_error(error)
}

pub unsafe fn send(cid: i32) -> Result<()> {
    ipc_syscall(cid, IPC_SEND)
}

pub unsafe fn send_noblock(cid: i32) -> Result<()> {
    ipc_syscall(cid, IPC_SEND | IPC_NOBLOCK)
}

pub unsafe fn recv(cid: i32) -> Result<()> {
    ipc_syscall(cid, IPC_RECV)
}

pub unsafe fn call(cid: i32) -> Result<()> {
    ipc_syscall(cid, IPC_SEND | IPC_RECV)
}
