use linked_list_allocator::LockedHeap;

#[global_allocator]
static ALLOCATOR: LockedHeap = LockedHeap::empty();

extern "C" {
    static __heap: u8;
    static __heap_end: u8;
}

pub fn init() {
    unsafe {
        let heap_start = &__heap as *const u8 as usize;
        let heap_end = &__heap_end as *const u8 as usize;
        ALLOCATOR.lock().init(heap_start, heap_end - heap_start);
    }
}
