use crate::utils::{VAddr, PAddr};
use crate::arch::{PAGE_SIZE, INITFS_BASE};
use crate::allocator::PAGE_POOL;

global_asm!(r#"
.globl initfs_image
initfs_image:
.align 0x10000
.incbin "../initfs.bin"
"#);

extern "C" {
    static initfs_image: u8;
}

pub fn pager(addr: VAddr) -> PAddr {
    let page = PAGE_POOL.alloc_or_panic();
    let offset = addr.as_usize() - INITFS_BASE.as_usize();

    unsafe {
        let from = VAddr::from_ptr(&initfs_image as *const u8).add(offset);
        page.copy_from(PAGE_SIZE, from, PAGE_SIZE);
    }

    page.paddr()
}

pub fn init() {

}