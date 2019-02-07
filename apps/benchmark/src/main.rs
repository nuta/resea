#![no_std]
#![feature(alloc)]
#![feature(asm)]

#[macro_use]
extern crate resea;
extern crate alloc;

use alloc::vec::Vec;
use resea::channel::{Channel, CId};


fn rdtscp() -> u64 {
    let rax: u64;
    let rdx: u64;
    let _cpu: u64;

    unsafe {
        asm!(
            "rdtscp"
            : "={rax}"(rax), "={rdx}"(rdx), "={rcx}"(_cpu)
        );
    }

    (rdx << 32) | rax
}

fn main() {
    println!("benchmark: starting");
    let memmgr = resea::idl::memmgr::Client::from_channel(Channel::from_cid(CId::new(1)));

    let mut results = Vec::new();
    for _ in 0..32 {
        let start = rdtscp();
        memmgr.nop().unwrap();
        let end = rdtscp();
        results.push(end - start);
    }

    results.sort();

    println!("benchmark: results -----------------------");
    println!("ipc latency: average={}, median={}, min={}, max={}",
        results.iter().sum::<u64>() as usize / results.len(),
        results[results.len() / 2],
        results.iter().min().unwrap(),
        results.iter().max().unwrap()
    );
}
