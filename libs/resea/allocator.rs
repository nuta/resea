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
    static __valloc: u8;
    static __valloc_end: u8;
}

pub struct AllocatedPage {
    pub addr: usize,
    pub num_pages: usize,
}

impl AllocatedPage {
    pub fn as_mut_ptr(&mut self) -> *mut u8 {
        self.addr as *mut u8
    }

    pub fn as_page_payload(&self) -> Page {
        Page::new(self.addr, self.num_pages * PAGE_SIZE)
    }
}

pub struct FreeArea {
    addr: usize,
    num_pages: usize,
}

pub struct PageAllocator {
    free_area: Vec<FreeArea>,
}

impl PageAllocator {
    pub fn new(free_area_start: usize, free_area_size: usize) -> PageAllocator {
        let free_area = FreeArea {
            addr: free_area_start,
            num_pages: free_area_size / PAGE_SIZE,
        };

        PageAllocator {
            free_area: vec![free_area],
        }
    }

    pub fn allocate(&mut self, num_pages: usize) -> AllocatedPage {
        while let Some(free_area) = self.free_area.pop() {
            if num_pages <= free_area.num_pages {
                let remaining_pages = free_area.num_pages - num_pages;
                if remaining_pages > 0 {
                    self.free_area.push(FreeArea {
                        addr: free_area.addr + PAGE_SIZE * num_pages,
                        num_pages: remaining_pages,
                    });
                }
                return AllocatedPage {
                    addr: free_area.addr,
                    num_pages,
                };
            }
        }

        panic!("out of memory");
    }
}

static VALLOC_NEXT_ADDR: LazyStatic<RefCell<usize>> = LazyStatic::new();
pub fn valloc(num_pages: usize) -> usize {
    // TODO:
    let addr = *VALLOC_NEXT_ADDR.borrow();
    *VALLOC_NEXT_ADDR.borrow_mut() = addr + num_pages * crate::PAGE_SIZE;
    addr
}

pub fn init() {
    unsafe {
        let heap_start = &__heap as *const u8 as usize;
        let heap_end = &__heap_end as *const u8 as usize;
        ALLOCATOR.lock().init(heap_start, heap_end - heap_start);

        let valloc_start = &__valloc as *const u8 as usize;
        let _valloc_end = &__valloc_end as *const u8 as usize;
        VALLOC_NEXT_ADDR.init(RefCell::new(valloc_start));
    }
}
