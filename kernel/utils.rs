use crate::arch::KERNEL_BASE_ADDR;
use core::cell::UnsafeCell;
use core::marker::Sync;
use core::ops::{Deref, DerefMut};
use core::ptr::NonNull;
use core::sync::atomic::{AtomicBool, Ordering};
use core::intrinsics::atomic_cxchg;

#[macro_export]
macro_rules! try_or_return_error {
    ($e:expr) => {
        match $e {
            Ok(v) => v,
            Err(err) => return err,
        }
    };
}

#[macro_export]
macro_rules! unwrap_or_return {
    ($e:expr, $err:expr) => {
        match $e {
            Some(v) => v,
            None => return $err,
        }
    };
}

/// Aligns down.
///
/// ```
/// assert_eq!(align_down(0x1fff, 0x1000), 0x1000);
/// assert_eq!(align_down(0x2000, 0x1000), 0x2000);
/// assert_eq!(align_down(0x2001, 0x1000), 0x2000);
/// ```
pub const fn align_down(value: usize, align: usize) -> usize {
    value & !(align - 1)
}

/// Atomic compare and swap. Returns `true` if the value is
/// successfully swapped.
pub fn compare_and_swap<T>(ptr: *mut T, old: T, new: T) -> bool {
    unsafe {
        atomic_cxchg(ptr, old, new).1
    }
}

/// A yet another lazy_static implementation featuring:
/// - The *explicit* initialization.
///
/// TODO: No overheads after an initialization like static_branch in Linux
///       kernel.
pub struct LazyStatic<T> {
    // TODO: Replace with mem::uninitialized() to remove dereference overhead.
    inner: UnsafeCell<Option<T>>,
}

impl<T> LazyStatic<T> {
    pub const fn new() -> LazyStatic<T> {
        LazyStatic {
            inner: UnsafeCell::new(None),
        }
    }

    /// `self` is immutable; assuming that the initialization is performed only once.
    #[inline]
    pub fn init(&self, initializer: T) {
        unsafe {
            *self.inner.get() = Some(initializer);
        }
    }
}

impl<T> Deref for LazyStatic<T> {
    type Target = T;

    #[inline]
    fn deref(&self) -> &T {
        unsafe {
            match *self.inner.get() {
                Some(ref inner) => inner,
                None => panic!("uninitialized LazyStatic dereference"),
            }
        }
    }
}

impl<T> DerefMut for LazyStatic<T> {
    #[inline]
    fn deref_mut(&mut self) -> &mut T {
        unsafe {
            match *self.inner.get() {
                Some(ref mut inner) => inner,
                None => panic!("uninitialized LazyStatic dereference"),
            }
        }
    }
}

unsafe impl<T: Sized + Send> Sync for LazyStatic<T> {}
unsafe impl<T: Sized + Send> Send for LazyStatic<T> {}

/// A pointer trait.
pub trait Pointer: Copy + FromVAddr {
    fn vaddr(&self) -> VAddr;
}

impl<T> Pointer for NonNull<T> {
    fn vaddr(&self) -> VAddr {
        VAddr::from_ptr(self.as_ptr())
    }
}

/// Virtual address.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(transparent)]
pub struct VAddr(pub usize);

impl VAddr {
    #[inline]
    pub const fn new(addr: usize) -> VAddr {
        VAddr(addr)
    }

    #[inline]
    pub const fn from_ptr<T>(ptr: *const T) -> VAddr {
        VAddr::new(ptr as usize)
    }

    #[inline]
    pub const fn null() -> VAddr {
        VAddr(0)
    }

    #[inline]
    pub fn add(self, offset: usize) -> VAddr {
        VAddr::new(self.0 + offset)
    }

    #[inline]
    pub fn sub(self, offset: usize) -> VAddr {
        VAddr::new(self.0 - offset)
    }

    #[inline]
    pub fn align_down(self, align: usize) -> VAddr {
        VAddr::new(align_down(self.0, align))
    }

    pub unsafe fn copy_from(self, self_len: usize, src: VAddr, copy_len: usize) {
        assert!(self_len >= copy_len);
        core::ptr::copy_nonoverlapping(src.as_ptr::<u8>(), self.as_ptr(), copy_len);
    }

    #[inline]
    pub fn as_usize(self) -> usize {
        self.0
    }

    #[inline]
    pub fn paddr(self) -> PAddr {
        PAddr::new(self.0 - KERNEL_BASE_ADDR)
    }

    #[inline]
    pub unsafe fn as_ptr<T>(self) -> *mut T {
        self.0 as *mut T
    }

    #[inline]
    pub unsafe fn as_ref<T>(&self) -> &T {
        &*self.as_ptr()
    }

    #[inline]
    pub unsafe fn as_mut<T>(&self) -> &mut T {
        &mut *self.as_ptr()
    }

    #[inline]
    pub unsafe fn as_nonnull<T>(self) -> NonNull<T> {
        NonNull::new_unchecked(self.as_ptr())
    }

    #[inline]
    pub fn cast<U: FromVAddr>(self) -> U {
        U::from_vaddr(self)
    }
}

pub trait FromVAddr {
    fn from_vaddr(addr: VAddr) -> Self;
}
impl<T> FromVAddr for NonNull<T> {
    fn from_vaddr(addr: VAddr) -> Self {
        unsafe { NonNull::new_unchecked(addr.as_ptr()) }
    }
}

/// Physical address.
#[derive(Clone, Copy)]
#[repr(transparent)]
pub struct PAddr(pub usize);

impl PAddr {
    #[inline]
    pub const fn new(addr: usize) -> PAddr {
        PAddr(addr)
    }

    #[inline]
    pub fn as_usize(self) -> usize {
        self.0
    }

    #[inline]
    pub fn as_vaddr(self) -> VAddr {
        VAddr::new(KERNEL_BASE_ADDR + self.0)
    }
}

pub struct SpinLock<T> {
    lock: AtomicBool,
    inner: UnsafeCell<T>,
}

pub struct LockGuard<'a, T> {
    rflags: u64,
    value: &'a mut T,
    lock: &'a AtomicBool,
}

impl<T> SpinLock<T> {
    pub const fn new(value: T) -> SpinLock<T> {
        SpinLock {
            lock: AtomicBool::new(false),
            inner: UnsafeCell::new(value),
        }
    }

    pub fn lock(&self) -> LockGuard<T> {
        let rflags = load_rflags();
        unsafe {
            #[cfg(not(test))]
            disable_interrupts();
        }

        loop {
            if !self.lock.compare_and_swap(false, true, Ordering::SeqCst) {
                break;
            }

            // FIXME: remove this or cfg(single_core)
            oops!("deadlock!");

            pause();
        }

        // Now we have the lock.
        LockGuard {
            value: unsafe { &mut *self.inner.get() },
            lock: &self.lock,
            rflags,
        }
    }
}

impl<'a, T> Drop for LockGuard<'a, T> {
    fn drop(&mut self) {
        self.lock.store(false, Ordering::SeqCst);
        unsafe {
            #[cfg(not(test))]
            restore_rflags(self.rflags);
        }
    }
}

impl<'a, T> Deref for LockGuard<'a, T> {
    #[inline(always)]
    type Target = T;
    fn deref(&self) -> &T {
        self.value
    }
}

impl<'a, T> DerefMut for LockGuard<'a, T> {
    #[inline(always)]
    fn deref_mut(&mut self) -> &mut T {
        self.value
    }
}

unsafe impl<T: Send> Sync for SpinLock<T> {}
unsafe impl<T: Send> Send for SpinLock<T> {}

#[inline(always)]
fn pause() {
    unsafe {
        asm!("pause" :::: "volatile");
    }
}

#[inline(always)]
pub unsafe fn disable_interrupts() {
    asm!("cli" ::: "memory" : "volatile");
}

#[inline(always)]
pub fn load_rflags() -> u64 {
    unsafe {
        let rflags;
        asm!("pushfq; pop $0" : "={rax}"(rflags) ::: "volatile");
        rflags
    }
}

#[inline(always)]
pub unsafe fn restore_rflags(rflags: u64) {
    asm!("push $0; popfq" :: "{rax}"(rflags) :: "volatile");
}
