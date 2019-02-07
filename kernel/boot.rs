use crate::arch;
use crate::memory;
use crate::debug;
use crate::process;
use crate::channel;
use crate::thread;
use crate::initfs;
use crate::utils::VAddr;
use crate::process::{KERNEL_PROCESS, Pager};
use crate::thread::ThreadState;
use crate::memory::{VmArea, PageFlags};

pub fn idle() {
    loop {
        arch::idle();
    }
}

/// Create the first user process from the initfs.
fn userland() {
    let proc = process::Process::new();
    let thread1 = thread::Thread::new(proc, arch::INITFS_BASE, VAddr::null(), 0);

    // Create a channel @1 linked to the kernel server.
    let kernel_ch = KERNEL_PROCESS.channels().alloc().expect("failed to allocate a channel");
    let ch = proc.channels().alloc().expect("failed to allocate a channel");
    channel::link(kernel_ch, ch);

    // Set up the pager.
    let mut flags = PageFlags::new();
    flags.set_writable();
    flags.set_user();

    proc.add_vmarea(VmArea::new(
        arch::INITFS_BASE,
        arch::INITFS_IMAGE_SIZE,
        flags,
        Pager::InitfsPager
    ));

    proc.add_vmarea(VmArea::new(
        arch::STRAIGHT_MAP_BASE,
        core::usize::MAX,
        flags,
        Pager::StraightMappingPager
    ));

    thread1.set_state(ThreadState::Runnable);
    thread::enqueue(thread1);
}

pub fn boot() {
    printk!("Booting Resea...");
    debug::init();
    memory::init();
    arch::init();
    channel::init();
    process::init();
    thread::init();
    initfs::init();

    userland();

    // Perform the very first context switch. The current context will be a
    // CPU-local idle thread.
    thread::switch();

    // Now we're in the idle thread context.
    idle();
}
