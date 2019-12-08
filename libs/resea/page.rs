use core::cell::Cell;
use crate::allocator::{WRITABLE_PAGE_ALLOCATOR, UNMAPPED_PAGE_ALLOCATOR};
use crate::utils::align_up;
use crate::PAGE_SIZE;

const DEFAULT_NUM_RX_PAGES_MAX: usize = 64; /* 256KiB */

#[repr(C, packed)]
pub struct PageBase {
    pub addr: usize,
    pub num_pages: usize,
}

impl PageBase {
    pub fn allocate() -> PageBase {
        let num_pages = DEFAULT_NUM_RX_PAGES_MAX;
        let addr =  UNMAPPED_PAGE_ALLOCATOR.borrow_mut().allocate(num_pages);
        PageBase {
            addr,
            num_pages,
        }
    }

    pub fn free(&mut self) {
        UNMAPPED_PAGE_ALLOCATOR.borrow_mut().free(self.addr, self.num_pages);
        self.invalidate();
    }

    pub fn invalidate(&mut self) {
        unsafe { assert_ne!(self.addr, 0); }
        self.addr = 0;
    }
}

impl Drop for PageBase {
    fn drop(&mut self) {
        if self.addr != 0 {
            self.free();
        }
    }
}

#[repr(C, packed)]
#[derive(Clone, Copy)]
pub struct RawPage {
    pub addr: usize,
    pub len: usize,
}

impl RawPage {
    pub const fn new(addr: usize, len: usize) -> RawPage {
        RawPage { addr, len }
    }

    pub fn into_page(self, mut page_base: PageBase) -> Page {
        unsafe { assert_eq!(self.addr, page_base.addr); }
        // TODO: Free remaining pages (self.num_pages() - DEFAULT_NUM_RX_PAGES_MAX).
        page_base.invalidate();
        Page::from_raw_page(self)
    }
}

pub struct Page {
    raw: RawPage,
    moved: Cell<bool>,
}

impl Page {
    pub fn new(len: usize) -> Page {
        let num_pages = align_up(len, PAGE_SIZE) / PAGE_SIZE;
        let addr = WRITABLE_PAGE_ALLOCATOR.borrow_mut().allocate(num_pages);
        Page {
            raw: RawPage::new(addr, len),
            moved: Cell::new(false),
        }
    }

    pub fn from_addr(addr: usize, len: usize) -> Page {
        Page {
            raw: RawPage::new(addr, len),
            moved: Cell::new(false),
        }
    }

    pub fn from_bytes(bytes: &[u8]) -> Page {
        let mut page = Page::new(bytes.len());
        page.copy_from_slice(bytes);
        page
    }

    pub const fn from_raw_page(raw_page: RawPage) -> Page {
        Page { raw: raw_page, moved: Cell::new(false) }
    }

    pub unsafe fn as_raw_page(&self) -> RawPage {
        self.raw
    }

    pub unsafe fn mark_as_moved(&self) {
        self.moved.replace(true);
    }

    pub fn is_empty(&self) -> bool {
        self.len() == 0
    }

    pub fn addr(&self) -> usize {
        self.raw.addr
    }

    pub fn len(&self) -> usize {
        self.raw.len
    }

    pub fn num_pages(&self) -> usize {
        align_up(self.len(), PAGE_SIZE) / PAGE_SIZE
    }

    pub fn as_slice_mut<T>(&mut self) -> &mut [T] {
        unsafe {
            let num = self.len() / core::mem::size_of::<T>();
            core::slice::from_raw_parts_mut(self.as_mut_ptr(), num)
        }
    }

    pub fn as_slice<T>(&self) -> &[T] {
        unsafe {
            let num = self.len() / core::mem::size_of::<T>();
            core::slice::from_raw_parts(self.as_ptr(), num)
        }
    }

    pub fn as_bytes_mut(&mut self) -> &mut [u8] {
        self.as_slice_mut()
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.as_slice()
    }

    pub unsafe fn as_ptr<T>(&self) -> *const T {
        self.raw.addr as *const T
    }

    pub unsafe fn as_mut_ptr<T>(&self) -> *mut T {
        self.raw.addr as *mut T
    }

    pub fn copy_from_slice(&mut self, data: &[u8]) {
        let len = data.len();

        // This constraint is essential in order to track the correct # of pages
        // in the writable page allocator.
        assert_eq!(align_up(len, PAGE_SIZE) / PAGE_SIZE, self.num_pages());

        self.raw.len = len;
        self.as_bytes_mut().copy_from_slice(data);

        // TODO: Fill the remaining bytes with zeros in order to prevent
        //       information leak.
    }
}

impl Clone for Page {
    fn clone(&self) -> Page {
        let mut new_page = Page::new(self.num_pages());
        new_page.copy_from_slice(self.as_bytes());
        new_page
    }
}

impl Drop for Page {
    fn drop(&mut self) {
        let mut allocator = if self.moved.get() {
            UNMAPPED_PAGE_ALLOCATOR.borrow_mut()
        } else {
            WRITABLE_PAGE_ALLOCATOR.borrow_mut()
        };

        allocator.free(self.addr(), self.num_pages());
    }
}
