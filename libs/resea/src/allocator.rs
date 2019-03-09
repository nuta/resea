use core::alloc::{GlobalAlloc, Layout};

pub struct MyAllocator {}

static mut NEXT_MALLOC: u64 = 0x0310_0000;

unsafe impl GlobalAlloc for MyAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        let addr = NEXT_MALLOC;
        NEXT_MALLOC += layout.size() as u64;
        addr as *mut u8
    }

    unsafe fn dealloc(&self, _ptr: *mut u8, _layout: Layout) {}
}
