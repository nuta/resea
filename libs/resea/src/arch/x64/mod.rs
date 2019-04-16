pub mod syscalls;

global_asm!(include_str!("./memcpy.S"));
