use crate::lazy_static::LazyStatic;
use crate::prelude::*;
use crate::PAGE_SIZE;
use core::cell::RefCell;
use linked_list_allocator::LockedHeap;

#[cfg_attr(target_os = "resea", global_allocator)]
static ALLOCATOR: LockedHeap = LockedHeap::empty();

extern "C" {
    static __heap: u8;
    static __heap_end: u8;
    static __writable_pages: u8;
    static __writable_pages_end: u8;
    static __unmapped_pages: u8;
    static __unmapped_pages_end: u8;
}

pub struct AllocatedPage {
    pub addr: usize,
    pub num_pages: usize,
}

pub struct FreeArea {
    addr: usize,
    num_pages: usize,
}

pub struct PageAllocator {
    free_area: Vec<FreeArea>,
    num_free: usize,
}

impl PageAllocator {
    pub fn new(free_area_start: usize, free_area_size: usize) -> PageAllocator {
        let num_pages = free_area_size / PAGE_SIZE;
        let free_area = FreeArea {
            addr: free_area_start,
            num_pages,
        };

        PageAllocator {
            free_area: vec![free_area],
            num_free: num_pages,
        }
    }

    pub fn allocate(&mut self, num_pages: usize) -> usize {
        while let Some(free_area) = self.free_area.pop() {
            if num_pages <= free_area.num_pages {
                let remaining_pages = free_area.num_pages - num_pages;
                if remaining_pages > 0 {
                    self.free_area.push(FreeArea {
                        addr: free_area.addr + PAGE_SIZE * num_pages,
                        num_pages: remaining_pages,
                    });
                }

                self.num_free -= num_pages;
                return free_area.addr;
            }
        }

        panic!("out of memory");
    }

    pub fn free(&mut self, addr: usize, num_pages: usize) {
        self.free_area.push(FreeArea { addr, num_pages });
        self.num_free += num_pages;
    }

    pub fn num_free(&self) -> usize {
        self.num_free
    }
}

pub(crate) static WRITABLE_PAGE_ALLOCATOR: LazyStatic<RefCell<PageAllocator>> = LazyStatic::new();
pub(crate) static UNMAPPED_PAGE_ALLOCATOR: LazyStatic<RefCell<PageAllocator>> = LazyStatic::new();

pub fn init() {
    unsafe {
        let heap_start = &__heap as *const u8 as usize;
        let heap_end = &__heap_end as *const u8 as usize;
        ALLOCATOR.lock().init(heap_start, heap_end - heap_start);

        let writable_pages_start = &__writable_pages as *const u8 as usize;
        let writable_pages_end = &__writable_pages_end as *const u8 as usize;
        WRITABLE_PAGE_ALLOCATOR.init(
            RefCell::new(
                PageAllocator::new(writable_pages_start, writable_pages_end)));

        let unmapped_pages_start = &__unmapped_pages as *const u8 as usize;
        let unmapped_pages_end = &__unmapped_pages_end as *const u8 as usize;
        UNMAPPED_PAGE_ALLOCATOR.init(
            RefCell::new(
                PageAllocator::new(unmapped_pages_start, unmapped_pages_end)));
    }
}
