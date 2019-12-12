use resea::prelude::*;
use resea::arch::{
    thread_info::get_thread_info, syscall::{SYSCALL_IPC, IPC_SEND, IPC_RECV}
};
use resea::server::connect_to_server;
use resea::idl::benchmark;
use resea::idl::timer;

static KERNEL_SERVER: Channel = Channel::from_cid(2);
const NUM_ITERS: usize = 500;

#[inline(always)]
fn cpu_cycle_counter() -> u64 {
    unsafe {
        let eax: u32;
        let edx: u32;
        asm!("rdtscp" : "={eax}"(eax), "={edx}"(edx) ::: "volatile");
        ((edx as u64) << 32) | (eax as u64)
    }
}

fn ipc(cid: i32) {
    unsafe {
        let _error: i32;
        asm!(
            "syscall"
            : "={eax}"(_error)
            : "{eax}"(SYSCALL_IPC | IPC_SEND | IPC_RECV),
              "{rdi}"(cid)
            : "memory", "rdi", "rsi", "rdx", "rcx", "r8", "r9", "r10", "r11"
            : "volatile"
        );
    }
}

#[no_mangle]
pub fn main() {
    warn!("connecting to a benchmark server...");
    let server = connect_to_server(benchmark::INTERFACE_ID)
        .expect("failed to connect to a benchmark server");

    // Wait for 5 seconds.
    let timer_ch = Channel::create().unwrap();
    let timer_client_ch = Channel::create().unwrap();
    timer_client_ch.transfer_to(&timer_ch).unwrap();
    timer::call_create(&KERNEL_SERVER, timer_client_ch, 3000, 0).unwrap();
    timer_ch.recv().unwrap();

    info!("starting benchmark (round-trip IPC)...");
    let mut results = vec![0; NUM_ITERS];
    let thread_info = unsafe { get_thread_info() };
    let server_cid = server.cid();
    for i in 0..NUM_ITERS {
        thread_info.ipc_buffer.header = benchmark::NOP_MSG;
        let start = cpu_cycle_counter();
        ipc(server_cid);
        results[i] = cpu_cycle_counter() - start;
    }

    results.sort();
    info!("finished benchmark!");
    info!("round-trip IPC: lowest = {}, median = {}, highest = {} (CPU cycles)",
        results[0], results[results.len() / 2], results[results.len() - 1]);
}
