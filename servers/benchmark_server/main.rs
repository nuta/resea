use resea::prelude::*;
use resea::arch::{
    thread_info::get_thread_info, syscall::{SYSCALL_IPC, IPC_SEND, IPC_RECV}
};
use resea::mem::transmute;
use resea::server::publish_server;
use resea::idl::benchmark;
use resea::idl::server::{CONNECT_REPLY_MSG, ConnectReplyMsg};

#[inline(always)]
fn ipc_recv(cid: i32) {
    unsafe {
        let _error: i32;
        asm!(
            "syscall"
            : "={eax}"(_error)
            : "{eax}"(SYSCALL_IPC | IPC_RECV),
              "{rdi}"(cid)
            : "memory", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
            : "volatile"
        );
    }
}

#[inline(always)]
fn ipc_send(cid: i32) {
    unsafe {
        let _error: i32;
        asm!(
            "syscall"
            : "={eax}"(_error)
            : "{eax}"(SYSCALL_IPC | IPC_SEND),
              "{rdi}"(cid)
            : "memory", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
            : "volatile"
        );
    }
}

#[no_mangle]
pub fn main() {
    let server_ch = Channel::create().unwrap();
    publish_server(benchmark::INTERFACE_ID, &server_ch).unwrap();

    // Handle a server.connect request.
    let m = server_ch.recv().unwrap();
    let reply_to = Channel::from_cid(m.from);
    let client_ch = Channel::create().unwrap();
    client_ch.transfer_to(&server_ch).unwrap();
    let mut reply = unsafe { transmute::<Message, ConnectReplyMsg>(m) };
    reply.header = CONNECT_REPLY_MSG;
    reply.ch = client_ch.cid();
    reply.interface = benchmark::INTERFACE_ID;
    reply_to.send(&unsafe { transmute::<ConnectReplyMsg, Message>(reply) }).unwrap();

    info!("ready");

    // Handle benchmark (round-trip IPC) messages.
    let thread_info = unsafe { get_thread_info() };
    let server_ch_cid = server_ch.cid();
    loop {
        ipc_recv(server_ch_cid);
        let from = thread_info.ipc_buffer.from;
        ipc_send(from);
    }
}
