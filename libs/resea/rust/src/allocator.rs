use crate::capi::{free, malloc, size_t};
use alloc::alloc::{GlobalAlloc, Layout};

#[alloc_error_handler]
#[cfg(not(test))]
fn alloc_error(layout: Layout) -> ! {
    panic!("alloc_error_handler: {:?}", layout);
}

struct GlobalAllocator();

impl GlobalAllocator {
    const fn new() -> GlobalAllocator {
        GlobalAllocator()
    }
}

unsafe impl GlobalAlloc for GlobalAllocator {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        assert!(layout.align() <= 16);
        let ptr = malloc(layout.size() as size_t);
        if ptr.is_null() {
            panic!("out of memory");
        }
        ptr
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        free(ptr);
    }
}

#[global_allocator]
static GLOBAL_ALLOCATOR: GlobalAllocator = GlobalAllocator::new();
