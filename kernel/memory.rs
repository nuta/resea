use crate::arch::{current, PAGE_SIZE};
use crate::process::Pager;
use crate::thread::kill_current;
use crate::utils::{VAddr, PAddr};
use crate::collections::Link;
use crate::allocator::{Allocator, Ref, OBJ_POOL};
use crate::utils::LazyStatic;
use crate::server;
use crate::printk;
use crate::stats;
use crate::initfs;

/// A region in the virtual memory space.
pub struct VmArea {
    pub next: Link<Ref<VmArea>>,
    addr: VAddr,
    len: usize,
    flags: PageFlags,
    pager: Pager,
}

static VMAREA_ALLOCATOR: LazyStatic<Allocator<'static, VmArea>> = LazyStatic::new();

impl VmArea {
    pub fn new(addr: VAddr, len: usize, flags: PageFlags, pager: Pager) -> Ref<VmArea> {
        VMAREA_ALLOCATOR.alloc_init_or_panic(VmArea {
            next: Link::null(),
            addr,
            len,
            flags,
            pager,
        })
    }

    #[inline]
    pub fn allows_requested_flags(&self, requested: PageFaultFlags) -> bool {
        if requested.caused_by_user() && !self.flags.user() {
            return false;
        }

        if requested.tried_to_write() && !self.flags.writable() {
            return false;
        }

        true
    }

    #[inline]
    pub fn pager(&self) -> &Pager {
        &self.pager
    }

    #[inline]
    pub fn include_addr(&self, addr: VAddr) -> bool {
        let base = self.addr.as_usize();
        base <= addr.as_usize() && addr.as_usize() < base.saturating_add(self.len)
    }
}

// Note that all present pages are readable.
const PAGE_FLAG_USER: u8 = 1 << 1;
const PAGE_FLAG_WRITABLE: u8 = 1 << 2;

#[derive(Clone, Copy)]
pub struct PageFlags(u8);

impl PageFlags {
    #[inline]
    pub fn new() -> PageFlags {
        PageFlags(0)
    }

    #[inline]
    pub fn from_u8(value: u8) -> PageFlags {
        PageFlags(value)
    }

    #[inline]
    pub fn user(self) -> bool {
        self.0 & PAGE_FLAG_USER != 0
    }

    #[inline]
    pub fn writable(self) -> bool {
        self.0 & PAGE_FLAG_WRITABLE != 0
    }

    #[inline]
    pub fn set_writable(&mut self) {
        self.0 |= PAGE_FLAG_WRITABLE;
    }

    #[inline]
    pub fn set_user(&mut self) {
        self.0 |= PAGE_FLAG_USER;
    }
}

const PAGE_FAULT_FLAG_ALREADY_PRESENT: u8 = 1 << 0;
const PAGE_FAULT_FLAG_CAUSED_BY_USER: u8 = 1 << 1;
const PAGE_FAULT_FLAG_WRITE: u8 = 1 << 2;

#[derive(Clone, Copy)]
pub struct PageFaultFlagsBuilder(u8);

impl PageFaultFlagsBuilder {
    #[inline]
    pub fn new() -> PageFaultFlagsBuilder {
        PageFaultFlagsBuilder(0)
    }

    #[inline]
    pub fn present(self, value: bool) -> PageFaultFlagsBuilder {
        if value {
            PageFaultFlagsBuilder(self.0 | PAGE_FAULT_FLAG_ALREADY_PRESENT)
        } else {
            PageFaultFlagsBuilder(self.0 & !PAGE_FAULT_FLAG_ALREADY_PRESENT)
        }
    }

    #[inline]
    pub fn user(self, value: bool) -> PageFaultFlagsBuilder {
        if value {
            PageFaultFlagsBuilder(self.0 | PAGE_FAULT_FLAG_CAUSED_BY_USER)
        } else {
            PageFaultFlagsBuilder(self.0 & !PAGE_FAULT_FLAG_CAUSED_BY_USER)
        }
    }

    #[inline]
    pub fn write(self, value: bool) -> PageFaultFlagsBuilder {
        if value {
            PageFaultFlagsBuilder(self.0 | PAGE_FAULT_FLAG_WRITE)
        } else {
            PageFaultFlagsBuilder(self.0 & !PAGE_FAULT_FLAG_WRITE)
        }
    }

    #[inline]
    pub fn build(self) -> PageFaultFlags {
        PageFaultFlags(self.0)
    }
}

#[derive(Clone, Copy)]
pub struct PageFaultFlags(u8);

impl PageFaultFlags {
    #[inline]
    pub fn already_present(self) -> bool {
        self.0 & PAGE_FAULT_FLAG_ALREADY_PRESENT != 0
    }

    #[inline]
    pub fn caused_by_kernel(self) -> bool {
        !self.caused_by_user()
    }

    #[inline]
    pub fn caused_by_user(self) -> bool {
        self.0 & PAGE_FAULT_FLAG_CAUSED_BY_USER != 0
    }

    #[inline]
    pub fn tried_to_write(self) -> bool {
        self.0 & PAGE_FAULT_FLAG_WRITE != 0
    }
}

fn straight_mapping_pager(addr: VAddr) -> PAddr {
    PAddr::new(addr.as_usize())
}

pub fn page_fault_handler(addr: VAddr, requested: PageFaultFlags) {
    stats::PAGE_FAULT_TOTAL.increment();
    printk!("page_fault_handler: addr=%p", addr.as_usize());

    if requested.caused_by_kernel() {
        // This will never occur. NEVER!
        panic!("page fault in the kernel space");
    }

    if requested.already_present() {
        // Invalid access. For instance the user thread has tried to write to
        // readonly area.
        printk!("page fault: already present page access: addr=%p", addr.as_usize());
        kill_current();
    }

    // Use aligned address since the pager and `.link()` method of PageTable
    // implicitly assume that it is aligned by PAGE_SIZE. Drop `adddr` to
    // prevent a misuse.
    let aligned_vaddr = addr.align_down(PAGE_SIZE);
    drop(addr);

    // Look for the associated vm area.
    let process = current().process();
    for area in process.vmareas() {
        trace!("vmarea: base=%p, len=%x", area.addr.as_usize(), area.len);
        if area.include_addr(aligned_vaddr) && area.allows_requested_flags(requested) {
            // This area includes `addr`, allows the requested access, and
            // the page does not yet exist in the page table. Looks a valid
            // page fault. Let's handle this!

            // Ask the associated pager to fill a physical page.

            let paddr = match area.pager() {
                Pager::UserPager(ref pager) => server::user_pager(pager, aligned_vaddr),
                Pager::InitfsPager => initfs::pager(aligned_vaddr),
                Pager::StraightMappingPager => straight_mapping_pager(aligned_vaddr),
            };

            // Register the filled page with the page table.
            process.page_table().link(aligned_vaddr, paddr, 1, area.flags);

            // Now we've done what we have to do. Return to the exception
            // handler and resume the thread.
            return;
        }
    }

    // No appropriate vm area. The user thread must have accessed unallocated
    // area (e.g. NULL pointer deference).
    trace!("page fault: no appropriate vmarea: addr=%p", addr.as_usize());
    kill_current();
}

pub fn init() {
    VMAREA_ALLOCATOR.init(Allocator::new(&OBJ_POOL));
}