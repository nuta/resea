use crate::utils::{VAddr, PAddr};
use crate::arch::{PAGE_SIZE, INITFS_BASE};
use crate::allocator::PAGE_POOL;

pub fn pager(addr: VAddr) -> PAddr {
    let page = PAGE_POOL.alloc_or_panic();
    let offset = addr.as_usize() - INITFS_BASE.as_usize();

    unsafe {
        let from = VAddr::from_ptr(include_bytes!("../initfs.bin")).add(offset);
        page.copy_from(PAGE_SIZE, from, PAGE_SIZE);
    }

    page.paddr()
}

pub fn init() {

}
