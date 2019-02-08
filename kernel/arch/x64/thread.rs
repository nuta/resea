use super::KERNEL_STACK_SIZE;
use super::asm;
use super::gdt::{USER_CS64, USER_DS, USER_RPL};
use super::PAGE_SIZE;
use crate::allocator::Ref;
use crate::process::Process;
use crate::ipc::Payload;
use crate::thread::Thread;
use crate::utils::VAddr;
use core::mem::size_of;

global_asm!(include_str!("switch.S"));

const INITIAL_RFLAGS: u64 = 0x0202;

#[repr(C, packed)]
pub struct ArchThread {
    // IMPORTANT NOTE: Don't forget to update offsets in switch.S and handler.S
    //  as well!
    rip: u64,           // 0
    rsp: u64,           // 8
    rsp0: u64,          // 16: rsp0 == kernel_stack + (size of stack)
    channel_table: u64, // 24
    cr3: u64,           // 32
    rflags: u64,        // 40
    rbp: u64,           // 48
    r12: u64,           // 56
    r13: u64,           // 64
    r14: u64,           // 72
    r15: u64,           // 80
    rbx: u64,           // 88
}

impl ArchThread {
    pub fn new(process: Ref<Process>, start: VAddr, stack: VAddr, kernel_stack: VAddr, arg: usize) -> ArchThread {
        // Temporarily use the kernel stack to pass `start` and `arg` to
        // `enter_userspace`.
        let initial_rsp: *mut u64;
        unsafe {
            initial_rsp = kernel_stack
                    .add(KERNEL_STACK_SIZE)
                    .sub(size_of::<u64>() * 6)
                    .as_ptr();

            // arg
            initial_rsp.offset(0).write(arg as u64);

            // IRET frame: RIP, CS, RSP, RFLAGS, RSP, and SS
            initial_rsp
                .offset(1)
                .write(start.as_usize() as u64);
            initial_rsp
                .offset(2)
                .write(USER_CS64 as u64 | USER_RPL as u64);
            initial_rsp
                .offset(3)
                .write(INITIAL_RFLAGS);
            initial_rsp
                .offset(4)
                .write(stack.as_usize() as u64);
            initial_rsp
                .offset(5)
                .write(USER_DS as u64 | USER_RPL as u64);
        }

        ArchThread {
            rip: enter_userspace as *const () as u64,
            rsp0: kernel_stack.add(PAGE_SIZE * 8 /* FIXME */).as_usize() as u64,
            rsp: initial_rsp as u64,
            cr3: process.page_table().pml4().paddr_as_usize() as u64,
            channel_table: 0, // TODO:
            rflags: INITIAL_RFLAGS,
            rbp: 0,
            r12: 0,
            r13: 0,
            r14: 0,
            r15: 0,
            rbx: 0,
        }
    }
}

extern "C" {
    fn enter_userspace();
    fn x64_switch(prev: &ArchThread, next: &ArchThread, save_current: i32);
    fn x64_overwrite_context(thread: &ArchThread) -> i32;
    fn x64_send(
        header: u64,
        m0: u64,
        m1: u64,
        m2: u64,
        m3: u64,
        m4: u64,
        current: &ArchThread,
        dest: &ArchThread
    );
}

const MSR_GS_BASE: u32 = 0xc000_0101;

#[inline]
pub fn set_current_thread(thread: Ref<Thread>) {
    unsafe {
        asm::wrmsr(MSR_GS_BASE, thread.as_usize() as u64);
    }
}

#[inline]
pub fn current() -> Ref<Thread> {
    unsafe {
        Ref::from_ptr(asm::rdmsr(MSR_GS_BASE) as *mut Thread)
    }
}

/// Switches the page table.
pub fn switch_page_table(thread: Ref<Thread>) {
    unsafe {
        asm::set_cr3(thread.arch().cr3);
    }
}

/// Saves the context in `prev` and resume the `next`. context.
pub fn switch(prev: Ref<Thread>, next: Ref<Thread>, save_current: bool) {
    unsafe {
        switch_page_table(next);
        x64_switch(&prev.arch(), &next.arch(), save_current as i32);
    }
}

pub fn overwrite_context(thread: Ref<Thread>) -> bool {
    unsafe {
        if x64_overwrite_context(&thread.arch()) == 1 {
            true
        } else {
            false
        }
    }
}

pub fn send(
    header: Payload,
    m0: Payload,
    m1: Payload,
    m2: Payload,
    m3: Payload,
    m4: Payload,
    current: Ref<Thread>,
    dest: Ref<Thread>
) {
    unsafe {
        switch_page_table(dest);

        x64_send(
            header as u64,
            m0 as u64,
            m1 as u64,
            m2 as u64,
            m3 as u64,
            m4 as u64,
            &current.arch(),
            &dest.arch()
        );
    }
}
