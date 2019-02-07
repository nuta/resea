use crate::arch::KERNEL_BASE_ADDR;
use crate::utils::VAddr;
use core::marker::PhantomData;
use core::mem::size_of;

/// Physical address.
#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct PhyPtr<T>(pub usize, pub PhantomData<T>);

impl<T> PhyPtr<T> {
    pub const fn new(addr: usize) -> PhyPtr<T> {
        PhyPtr(addr, PhantomData)
    }

    pub fn from_vaddr(addr: VAddr) -> PhyPtr<T> {
        PhyPtr(addr.paddr().as_usize(), PhantomData)
    }

    #[inline]
    pub fn as_vaddr(&self) -> VAddr {
        VAddr::new(self.vaddr_as_usize())
    }

    #[inline]
    pub unsafe fn as_ptr<U>(&self) -> *const U {
        self.vaddr_as_usize() as *const U
    }

    #[inline]
    pub unsafe fn as_mut_ptr<U>(&self) -> *mut U {
        self.vaddr_as_usize() as *mut U
    }

    #[inline]
    pub fn add(&self, offset: usize) -> PhyPtr<T> {
        PhyPtr(self.0 + offset, PhantomData)
    }

    #[inline]
    pub fn paddr(&self) -> usize {
        self.0
    }

    #[inline]
    pub fn vaddr_as_usize(&self) -> usize {
        KERNEL_BASE_ADDR + self.0
    }

    #[inline]
    pub fn paddr_as_usize(&self) -> usize {
        self.0
    }

    #[inline]
    pub unsafe fn read(&self) -> T {
        self.read_at(0)
    }

    #[inline]
    pub unsafe fn write(&mut self, value: T) {
        self.write_at(0, value);
    }

    #[inline]
    pub unsafe fn read_at(&self, index: usize) -> T {
        let vaddr = self.as_vaddr().add(size_of::<T>() * index);
        core::ptr::read_volatile(vaddr.as_ptr())
    }

    #[inline]
    pub unsafe fn write_at(&mut self, index: usize, value: T) {
        let vaddr = self.as_vaddr().add(size_of::<T>() * index);
        core::ptr::write_volatile(vaddr.as_ptr(), value);
    }
}

impl<T> From<PhyPtr<T>> for usize {
    fn from(paddr: PhyPtr<T>) -> usize {
        KERNEL_BASE_ADDR + paddr.0 as usize
    }
}
