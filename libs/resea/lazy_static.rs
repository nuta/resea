use core::cell::UnsafeCell;
use core::ops::{Deref, DerefMut};

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
