use crate::arch::thread_info::get_thread_info;
use crate::allocator::valloc;

const DEFAULT_NUM_PAGES: usize = 64; /* 256KiB */

pub fn alloc_and_set_page_base() {
    unsafe {
        get_thread_info().page_addr = valloc(DEFAULT_NUM_PAGES);
        get_thread_info().num_pages = DEFAULT_NUM_PAGES;
    }
}
