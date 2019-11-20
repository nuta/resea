global_asm!(include_str!("start.S"));
global_asm!(include_str!("memcpy.S"));

mod main;
pub mod stackframe;
pub mod syscall;
pub mod thread_info;

pub const PAGE_SIZE: usize = 0x1000;

pub fn breakpoint() {
    unsafe {
        asm!("int $$3");
    }
}
