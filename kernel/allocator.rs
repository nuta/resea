use crate::arch::{
    OBJ_POOL_ADDR, OBJ_POOL_LEN, PAGE_POOL_ADDR, PAGE_POOL_LEN, PAGE_SIZE,
    KERNEL_STACK_SIZE, KERNEL_STACK_POOL_LEN, KERNEL_STACK_POOL_ADDR
};
use crate::utils::{FromVAddr, Pointer, VAddr};
use crate::collections::{List, Link};
use crate::stats;
use core::marker::PhantomData;
use core::ops::Deref;
use core::ptr::NonNull;
use core::sync::atomic::{AtomicUsize, Ordering};
use core::mem::size_of;

const FREED_MAGIC: u32 = 0xdead_beef;

struct FreeElement {
    next: Link<NonNull<FreeElement>>,
    free_magic: u32,
}

///
/// The fixed-length chunk memory pool.
///
/// ```
///   elements:
///     +-----------------------------------------------------------------+
///     |          objects allocated at least once       |  uninitlized   |
///     | (freed objects are moved into the `free_list`) |  memory space  |
///     +-----------------------------------------------------------------+
///                                                      ^                ^
///                                           elements_used       elements_max
/// ```
pub struct AllocationPool {
    elements: VAddr,
    /// # of elements which are not yet allocated.
    elem_size: usize,
    elements_max: usize,
    elements_used: AtomicUsize,
    free_list: List<NonNull<FreeElement>>,
}

impl AllocationPool {
    const fn new(base_addr: VAddr, pool_len: usize, elem_size: usize) -> AllocationPool {
        AllocationPool {
            elements: base_addr,
            elem_size,
            elements_max: pool_len / elem_size,
            elements_used: AtomicUsize::new(0),
            free_list: list_new!(FreeElement, next),
        }
    }

    pub fn alloc(&self) -> Option<VAddr> {
        if let Some(elem) = self.free_list.pop_front() {
            stats::POOL_ALLOCS.increment();
            return Some(VAddr::from_ptr(elem.as_ptr()));
        }

        let i = self.elements_used.fetch_add(1, Ordering::SeqCst);
        if i < self.elements_max {
            // Allocate from the uninitialized memory space.
            let addr = self.elements.as_usize() + self.elem_size * i;
            stats::POOL_ALLOCS.increment();
            Some(VAddr::new(addr))
        } else {
            None
        }
    }

    #[inline]
    pub fn alloc_or_panic(&self) -> VAddr {
        self.alloc().expect("run out of memory")
    }

    pub fn free(&self, obj: VAddr) {
        stats::POOL_FREES.increment();

        unsafe {
            self.free_list.push_back(obj.as_nonnull());
        }
    }
}

unsafe impl Sync for AllocationPool {}
unsafe impl Send for AllocationPool {}

/// A pointer points to an object in an allocation pool.
#[repr(transparent)]
pub struct Ref<T> {
    ptr: NonNull<T>,
}

impl<T> Ref<T> {
    #[inline]
    pub const fn new(ptr: NonNull<T>) -> Ref<T> {
        Ref {
            ptr,
        }
    }

    #[inline]
    pub fn from_vaddr(addr: VAddr) -> Ref<T> {
        unsafe { Ref::new(addr.as_nonnull()) }
    }

    #[inline]
    pub const fn from_ptr(ptr: *mut T) -> Ref<T> {
        unsafe { Ref::new(NonNull::new_unchecked(ptr)) }
    }

    #[inline]
    pub unsafe fn as_ptr(self) -> *mut T {
        self.ptr.as_ptr()
    }

    #[inline]
    pub unsafe fn as_usize(self) -> usize {
        self.as_ptr() as usize
    }
}

impl<T> Pointer for Ref<T> {
    fn vaddr(&self) -> VAddr {
        unsafe { VAddr::from_ptr(self.as_ptr()) }
    }
}

impl<T> FromVAddr for Ref<T> {
    fn from_vaddr(addr: VAddr) -> Ref<T> {
        Ref::from_vaddr(addr)
    }
}

impl<T> Copy for Ref<T> {}

impl<T> Clone for Ref<T> {
    fn clone(&self) -> Ref<T> {
        unsafe {
            Ref {
                ptr: NonNull::new_unchecked(self.ptr.as_ptr()),
            }
        }
    }
}

impl<T> Deref for Ref<T> {
    type Target = T;

    #[inline(always)]
    fn deref(&self) -> &T {
        unsafe { self.ptr.as_ref() }
    }
}

unsafe impl<T> Sync for Ref<T> {}
unsafe impl<T> Send for Ref<T> {}

pub struct Allocator<'a, T> {
    pool: &'a AllocationPool,
    _marker: PhantomData<T>,
}


impl<'a, T> Allocator<'a, T> {
    pub fn new(pool: &'a AllocationPool) -> Allocator<T> {
        // TODO: Make these assertions static (sadly, static assertion is
        //       not provided by rustc) to initialize allocators by `const fn`
        //       instead of LazyStatic.
        assert!(size_of::<T>() <= pool.elem_size);
        assert!(size_of::<FreeElement>() <= pool.elem_size);

        Allocator {
            pool,
            _marker: PhantomData,
        }
    }

    pub fn alloc(&self) -> Option<Ref<T>> {
        self.pool.alloc().map(|vaddr| {
            unsafe {
                // `vaddr` will never be NULL.
                Ref::new(NonNull::new_unchecked(vaddr.as_ptr()))
            }
        })
    }

    pub fn alloc_init_or_panic(&self, value: T) -> Ref<T> {
        let obj = self.alloc().expect("run out of memory");
        unsafe {
            core::ptr::write(obj.ptr.as_ptr(), value);
        }
        obj
    }

    pub fn alloc_init_or_panic2<F>(&self, f: F) -> Ref<T>
        where F: FnOnce(Ref<T>) -> T
    {
        let obj = self.alloc().expect("run out of memory");
        unsafe {
            core::ptr::write(obj.ptr.as_ptr(), f(obj));
        }
        obj
    }

    pub fn free(&self, obj: Ref<T>) {
        unsafe {
            // Ensure that this is not a double free.
            let elem_ptr: *mut FreeElement = obj.as_ptr() as *mut FreeElement;
            if (*elem_ptr).free_magic == FREED_MAGIC {
                // Here we use `oops!` instead of `bug!` because there may be
                // false positives; an object could be contain a value which
                // equals to `FREED_MAGIC`.
                oops!("possibly an uninitialized memory reference");
            }

            // Fill free'd area by 0xab to notice uninitialized memory references.
            core::ptr::write_bytes::<u8>(obj.as_ptr() as *mut u8, 0xab,
                self.pool.elem_size);

            // Write the magic number to detect double free later.
            (*elem_ptr).free_magic = FREED_MAGIC;

            // Put back the object into the allocation pool.
            self.pool.free(VAddr::new(obj.as_usize()))
        }
    }
}

unsafe impl<'a, T> Sync for Allocator<'a, T> {}
unsafe impl<'a, T> Send for Allocator<'a, T> {}

const OBJ_MAX_SIZE: usize = 512;
pub static OBJ_POOL: AllocationPool =
    AllocationPool::new(OBJ_POOL_ADDR, OBJ_POOL_LEN, OBJ_MAX_SIZE);
pub static PAGE_POOL: AllocationPool =
    AllocationPool::new(PAGE_POOL_ADDR, PAGE_POOL_LEN, PAGE_SIZE);
pub static KERNEL_STACK_POOL: AllocationPool =
    AllocationPool::new(KERNEL_STACK_POOL_ADDR, KERNEL_STACK_POOL_LEN, KERNEL_STACK_SIZE);

#[cfg(test)]
mod tests {
    use super::*;

    #[derive(Clone)]
    struct TestObject {
        a: i32,
        b: i32,
        c: i32,
        d: i32,
        e: i32,
    }

    #[test]
    fn test_allocator() {
        let base = VAddr::new(std::boxed::Box::new([0u8;1024]).as_ref() as *const u8 as usize);
        let elem_size = 64;
        assert!(elem_size >= std::mem::size_of::<TestObject>());

        let pool = AllocationPool::new(base, 0x1000, elem_size);
        let allocator: Allocator<TestObject> = Allocator::new(&pool);

        assert_eq!(pool.alloc().unwrap(), base.add(elem_size * 0));
        assert_eq!(pool.alloc().unwrap(), base.add(elem_size * 1));
        assert_eq!(allocator.alloc().unwrap().vaddr(), base.add(elem_size * 2));
        assert_eq!(allocator.alloc().unwrap().vaddr(), base.add(elem_size * 3));

        let elem1 = allocator.alloc().unwrap();
        assert_eq!(elem1.vaddr(), base.add(elem_size * 4));
        allocator.free(elem1);

        let elem2 = allocator.alloc().unwrap();
        assert_eq!(elem2.vaddr(), base.add(elem_size * 4));

        assert_eq!(allocator.alloc().unwrap().vaddr(), base.add(elem_size * 5));
    }
}
