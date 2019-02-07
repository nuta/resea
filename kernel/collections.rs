use crate::utils::{Pointer, SpinLock};
use core::cell::Cell;
use core::marker::Sync;
use core::ptr::NonNull;

#[derive(Clone)]
pub struct Link<T> {
    next: Cell<Option<NonNull<Link<T>>>>,
}

impl<T: Pointer> Link<T> {
    pub fn null() -> Link<T> {
        Link {
            next: Cell::new(None),
        }
    }
}

macro_rules! list_new {
    ($struct:ident, $field:ident) => {{
        #[allow(unused_unsafe)]
        unsafe {
            List::new(&(*(::core::ptr::null() as *const $struct)).$field as *const _ as usize)
        }
    }};
}

///
/// A Linked list. Unlike `std::collections::LinkedList`, this implementation
/// allows embedding multiple links [`Link`] into the object. It somewhat violates
/// Rust's ownership principle but we need this to decrease cache misses and footprint.
///
/// ```
///          struct T               struct T
///        +---------+            +---------+
///        |   ...   |            |   ...   |
///        |  next1  | ---------> |  next1  |
///        |   ...   |            |   ...   |
///        |   ...   |            |   ...   |
///        |  next2  |----+       |  next2  |
///        +---------+    |       +---------+
///                       |
///                       |             struct T               struct T
///                       |           +---------+            +---------+
///                       |           |   ...   |            |   ...   |
///                       |           |  next1  |            |  next1  |
///                       |           |   ...   |            |   ...   |
///                       |           |   ...   |            |   ...   |
///                       +---------> |  next2  | ---------> |  next2  |
///                                   +---------+            +---------+
/// ```
///
pub struct List<T: Pointer>(SpinLock<ListInner<T>>);

impl<T: Pointer> List<T> {
    pub const fn new(offset: usize) -> List<T> {
        List(SpinLock::new(ListInner::new(offset)))
    }

    #[inline]
    pub fn len(&self) -> usize {
        self.0.lock().len()
    }

    #[inline]
    pub fn push_back(&self, value: T) {
        self.0.lock().push_back(value);
    }

    #[inline]
    pub fn pop_front(&self) -> Option<T> {
        self.0.lock().pop_front()
    }

    #[inline]
    pub fn peek_front(&self) -> Option<T> {
        self.0.lock().peek_front()
    }

    #[inline]
    pub fn delete_front(&self) {
        self.0.lock().pop_front();
    }

    #[inline]
    pub fn iter(&self) -> Iter<T> {
        self.0.lock().iter()
    }
}

unsafe impl<T: Pointer + Sized + Send> Sync for List<T> {}
unsafe impl<T: Pointer + Sized + Send> Send for List<T> {}

struct ListInner<T: Pointer> {
    head: Option<NonNull<Link<T>>>,
    tail: Option<NonNull<Link<T>>>,
    len: usize,
    link_offset: usize,
}

impl<T: Pointer> ListInner<T> {
    const fn new(link_offset: usize) -> ListInner<T> {
        ListInner {
            head: None,
            tail: None,
            len: 0,
            link_offset,
        }
    }

    #[inline]
    fn len(&self) -> usize {
        self.len
    }

    fn push_back(&mut self, value: T) {
        unsafe {
            let new_link: *mut Link<T> = value.vaddr().add(self.link_offset).as_ptr();
            (*new_link).next.set(None);

            /*
            for elem in self.iter() {
                if elem.vaddr() == value.vaddr() {
                    oops!("appending a duplicated element into a list");
                    return;
                }
            }
            */

            if self.head.is_none() {
                self.head = Some(NonNull::new_unchecked(new_link));
            }

            if let Some(tail) = self.tail {
                tail.as_ref()
                    .next
                    .set(Some(NonNull::new_unchecked(new_link)));
            }

            self.tail = Some(NonNull::new_unchecked(new_link));
            self.len += 1;
        }
    }

    fn pop_front(&mut self) -> Option<T> {
        unsafe {
            let head = if let Some(head) = self.head {
                head
            } else {
                return None;
            };

            let head_link: *mut Link<T> = head.vaddr().as_ptr();
            let head_obj: T = head.vaddr().sub(self.link_offset).cast();
            self.head = (*head_link).next.get();
            self.len -= 1;

            if self.len == 0 {
                self.tail = None;
            }

            Some(head_obj)
        }
    }

    fn peek_front(&mut self) -> Option<T> {
        self.head.map(|head| {
            let obj: T = head.vaddr().sub(self.link_offset).cast();
            obj
        })
    }

    #[inline]
    pub fn iter(&self) -> Iter<T> {
        Iter {
            current: self.head,
            link_offset: self.link_offset,
        }
    }
}

pub struct Iter<T> {
    current: Option<NonNull<Link<T>>>,
    link_offset: usize,
}

impl<T: Pointer> Iterator for Iter<T> {
    type Item = T;

    fn next(&mut self) -> Option<T> {
        self.current.map(|current| {
            let obj_addr = current.vaddr().sub(self.link_offset);
            unsafe {
                self.current = current.as_ref().next.get();
            }
            T::from_vaddr(obj_addr)
        })
    }
}

impl<'a, T: Pointer> IntoIterator for &'a List<T> {
    type Item = T;
    type IntoIter = Iter<T>;

    #[inline]
    fn into_iter(self) -> Iter<T> {
        self.0.lock().iter()
    }
}

#[cfg(test)]
mod list_tests {
    use super::*;

    #[derive(Clone)]
    struct TestElem {
        next1: Link<NonNull<TestElem>>,
        next2: Link<NonNull<TestElem>>,
        value: i32,
    }

    #[test]
    fn test_list() {
        let mut elem1 = TestElem {
            next1: Link::null(),
            next2: Link::null(),
            value: 123,
        };

        let elem1_ptr: *mut TestElem = &mut elem1 as *mut _;

        let mut list1: ListInner<NonNull<TestElem>> = ListInner::new(0);
        assert_eq!(list1.len(), 0);

        let pop0 = list1.pop_front();
        assert_eq!(pop0, None);

        list1.push_back(unsafe { NonNull::new_unchecked(elem1_ptr) });
        assert_eq!(list1.len(), 1);

        let pop1 = list1.pop_front();
        assert!(pop1.is_some());
        assert_eq!(unsafe { pop1.unwrap().as_ref().value }, 123);
        assert_eq!(list1.len(), 0);

        let pop2 = list1.pop_front();
        assert!(pop2.is_none());
        assert_eq!(list1.len(), 0);
    }
}
