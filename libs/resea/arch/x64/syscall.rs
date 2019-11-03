use crate::error::Error;
use crate::message::Message;

const SYSCALL_IPC: u32 = 0;
const IPC_SEND: u32 = 1 << 8;
const IPC_RECV: u32 = 1 << 9;

unsafe fn ipc_syscall(cid: i32, ops: u32) -> Result<(), Error> {
    let error: i32;
    asm!(
        "syscall"
        : "={eax}"(error)
        : "{eax}"(SYSCALL_IPC | ops),
          "{rdi}"(cid)
        : "rsi", "rdx", "rcx", "r8", "r9", "r10" "r11"
    );

    if error < 0 {
        Err(core::mem::transmute::<u8, Error>(-error as u8))
    } else {
        Ok(())
    }
}

pub unsafe fn send(cid: i32) -> Result<(), Error> {
    ipc_syscall(cid, IPC_SEND)
}

pub unsafe fn recv(cid: i32) -> Result<(), Error> {
    ipc_syscall(cid, IPC_RECV)
}

pub unsafe fn call(cid: i32) -> Result<(), Error> {
    ipc_syscall(cid, IPC_SEND | IPC_RECV)
}
