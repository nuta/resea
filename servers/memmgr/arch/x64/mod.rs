global_asm!(include_str!("initfs.S"));

// Memmgr address space.
pub const FREE_MEMORY_START: usize = 0x0400_0000;
// TODO: Move KASan shadow memory to expand this size.
pub const FREE_MEMORY_SIZE: usize = 0x1000_0000 - FREE_MEMORY_START;

// App address space.
pub const THREAD_INFO_ADDR: usize = 0x00f1_b000;
pub const APP_IMAGE_START: usize = 0x0100_0000;
pub const APP_IMAGE_SIZE: usize = 0x0100_0000;
pub const APP_ZEROED_PAGES_START: usize = 0x0200_0000;
pub const APP_ZEROED_PAGES_SIZE: usize = 0x0b00_0000 - 0x0200_0000;
pub const APP_INITIAL_STACK_POINTER: usize = 0x0300_0000;
