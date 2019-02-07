use super::asm;
use super::KERNEL_BASE_ADDR;
use crate::allocator::PAGE_POOL;
use crate::arch::utils::PhyPtr;
use crate::memory::{page_fault_handler, PageFaultFlagsBuilder, PageFlags};
use crate::utils::{PAddr, VAddr};

const PAGE_SIZE: usize = 4096;
const PAGE_PRESENT: usize = 1 << 0;
const PAGE_WRITABLE: usize = 1 << 1;
const PAGE_USER: usize = 1 << 2;
const PAGE_ENTRY_NUM: usize = 512;
const PAGE_ATTRS: u64 = 0b0111;
const KERNEL_PML4_PADDR: PhyPtr<u64> = PhyPtr::new(0x0070_0000);

pub struct PageTable {
    // The physical address points to a PML4.
    pml4: PhyPtr<u64>,
}

impl PageTable {
    pub fn new() -> PageTable {
        let pml4 = PAGE_POOL.alloc_or_panic();

        // Fill the kernel space.
        unsafe {
            pml4.copy_from(PAGE_SIZE, KERNEL_PML4_PADDR.as_vaddr(), PAGE_SIZE);
        }

        PageTable {
            pml4: PhyPtr::from_vaddr(pml4),
        }
    }

    pub fn link(&self, vaddr: VAddr, paddr: PAddr, num: usize, flags: PageFlags) {
        assert!(vaddr.as_usize() < KERNEL_BASE_ADDR);

        let mut attrs = PAGE_PRESENT as u64;
        if flags.writable() {
            attrs |= PAGE_WRITABLE as u64;
        }
        if flags.user() {
            attrs |= PAGE_USER as u64;
        }

        loop {
            unsafe {
                let mut table: *mut u64 = self.pml4.as_mut_ptr();
                let mut level = 4;
                while level > 1 {
                    let index = ((vaddr.as_usize() >> (((level - 1) * 9) + 12)) & 0x1ff) as isize;
                    if table.offset(index).read() == 0 {
                        /* The PDPT, PD or PT is not allocated. Allocate it. */
                        let alloced_paddr = PAGE_POOL.alloc_or_panic().paddr().as_usize();
                        table.offset(index).write(alloced_paddr as u64);
                    }

                    // Update attributes.
                    let new_entry = (table.offset(index).read() & !PAGE_ATTRS) | attrs;
                    table.offset(index).write(new_entry);

                    // Go into the next level paging table.
                    let next_level_addr =
                        table.offset(index).read() as usize & 0x7fff_ffff_ffff_f000;
                    table = PhyPtr::<u64>::new(next_level_addr).as_mut_ptr();
                    level -= 1;
                }

                // `table` now points to the PT.
                let mut index = ((vaddr.as_usize() >> 12) & 0x1ff) as isize;
                let mut remaining = num;
                let mut offset: u64 = 0;
                let mut paddr = paddr.as_usize() as u64;
                while remaining > 0 && index < PAGE_ENTRY_NUM as isize {
                    printk!("link: %p -> %p (flags=0x%x)", vaddr.as_usize() + offset as usize, paddr, attrs);
                    table.offset(index).write(paddr | attrs);
                    asm::invlpg(vaddr.as_usize() as u64 + offset);
                    paddr += PAGE_SIZE as u64;
                    offset += PAGE_SIZE as u64;
                    remaining -= 1;
                    index += 1;
                }

                if remaining > 0 {
                    // The page spans across multiple PTs.
                    continue;
                }

                break;
            }
        }
    }

    pub fn resolve(&self, vaddr: VAddr) -> Option<PAddr> {
        unsafe {
            let mut table: *mut u64 = self.pml4.as_mut_ptr();
            let mut level = 4;
            while level > 1 {
                let index = ((vaddr.as_usize() >> (((level - 1) * 9) + 12)) & 0x1ff) as isize;
                if table.offset(index).read() == 0 {
                    return None;
                }

                // Go into the next level paging table.
                let next_level_addr =
                    table.offset(index).read() as usize & 0x7fff_ffff_ffff_f000;
                table = PhyPtr::<u64>::new(next_level_addr).as_mut_ptr();
                level -= 1;
            }

            // `table` now points to the PT.
            let index = ((vaddr.as_usize() >> 12) & 0x1ff) as isize;
            let entry = table.offset(index).read();
            if entry == 0 {
                None
            } else {
                Some(PAddr::new(entry as usize & 0x000f_ffff_ffff_f000))
            }
        }
    }

    #[inline]
    pub(super) fn pml4(&self) -> PhyPtr<u64> {
        self.pml4
    }
}

pub(super) fn handle_page_fault(error: u64) {
    let addr = unsafe { asm::get_cr2() };
    let present = (error >> 0) & 1 == 1;
    let write = (error >> 1) & 1 == 1;
    let user = (error >> 2) & 1 == 1;
    let rsvd = (error >> 3) & 1 == 1;

    if rsvd {
        panic!("#PF: RSVD bit violation");
    }

    let requested = PageFaultFlagsBuilder::new()
        .present(present)
        .user(user)
        .write(write)
        .build();

    page_fault_handler(VAddr::new(addr as usize), requested);
}