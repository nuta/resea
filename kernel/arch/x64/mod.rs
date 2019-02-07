#[macro_use]
mod apic;
mod asm;
mod cpu;
mod gdt;
mod idle;
mod idt;
mod interrupt;
mod exception;
mod ioapic;
mod paging;
mod pic;
mod print;
mod serial;
mod smp;
mod stack_frame;
mod syscall;
mod thread;
mod tss;
mod vga;
mod send;

use crate::utils::VAddr;

pub use self::apic::cpu_id;
pub use self::cpu::{halt, cpu};
pub use self::idle::idle;
pub use self::paging::PageTable;
pub use self::print::printchar;
pub use self::stack_frame::StackFrame;
pub use self::thread::{current, set_current_thread, switch, ArchThread, send, overwrite_context};
pub use self::send::return_from_kernel_server;
pub const TICK_HZ: usize = 1000;
pub const PAGE_SIZE: usize = 0x1000;
pub const KERNEL_BASE_ADDR: usize = 0xffff_8000_0000_0000;
pub const CPU_VAR_ADDR: VAddr = VAddr::new(0xffff_8000_00b0_0000);
pub const OBJ_POOL_ADDR: VAddr = VAddr::new(0xffff_8000_0100_0000);
pub const OBJ_POOL_LEN: usize = 0x80_0000;
pub const PAGE_POOL_ADDR: VAddr = VAddr::new(0xffff_8000_0180_0000);
pub const PAGE_POOL_LEN: usize = 0x80_0000;
pub const INITFS_BASE: VAddr = VAddr::new(0x0010_0000);
pub const INITFS_IMAGE_SIZE: usize = 0x0010_0000;
pub const STRAIGHT_MAP_BASE: VAddr = VAddr::new(0x0200_0000);

/// boot.S calls this function.
#[no_mangle]
pub fn init_x64() {
    unsafe {
        vga::clear_screen();
        serial::init();
    }

    crate::boot::boot();
}

pub fn init() {
    unsafe {
        pic::init();
        apic::init();
        gdt::init();
        tss::init();
        idt::init();
        smp::init();
        apic::init_timer();
        syscall::init();
    }
}
