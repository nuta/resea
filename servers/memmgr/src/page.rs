use core::sync::atomic::{AtomicUsize, Ordering};

pub const PAGE_SIZE: usize = 4096;
static NEXT_PAGE: AtomicUsize = AtomicUsize::new(0x0400_0000);

pub fn alloc_page() -> *mut u8 {
    let addr = NEXT_PAGE.fetch_add(PAGE_SIZE, Ordering::SeqCst);
    addr as *mut u8
}
