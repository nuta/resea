use linked_list_allocator::LockedHeap;

#[global_allocator]
static ALLOCATOR: LockedHeap = LockedHeap::empty();

pub fn init() {
    // FIXME:
    let heap_start = 0x400_0000;
    let heap_size  = 0x100_0000;
    unsafe {
        ALLOCATOR.lock().init(heap_start, heap_size);
    }
}
