use resea::PAGE_SIZE;
use resea::message::Page;
use resea::std::vec::Vec;

pub struct AllocatedPage {
    pub paddr: usize,
    pub num_pages: usize,
}

impl AllocatedPage {
    pub fn as_mut_ptr(&mut self) -> *mut u8 {
        self.paddr as *mut u8
    }

    pub fn as_page_payload(&self) -> Page {
        Page::new(self.paddr, self.num_pages)
    }
}

pub struct FreeArea {
    paddr: usize,
    num_pages: usize,
}

pub struct PageAllocator {
    free_area: Vec<FreeArea>,
}

impl PageAllocator {
    pub fn new(free_area_start: usize, free_area_size: usize) -> PageAllocator {
        let free_area = FreeArea {
            paddr: free_area_start,
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
                        paddr: free_area.paddr + PAGE_SIZE * num_pages,
                        num_pages: remaining_pages,
                    });
                }
                return AllocatedPage {
                    paddr: free_area.paddr,
                    num_pages,
                };
            }
        };

        panic!("out of memory");
    }
}
