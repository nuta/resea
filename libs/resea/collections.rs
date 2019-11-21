#![allow(clippy::implicit_hasher)]
#![allow(clippy::new_without_default)]

use alloc::collections::btree_map::Iter;
pub use alloc::collections::{
    BTreeMap, BTreeSet, BinaryHeap, LinkedList, VecDeque
};
pub use alloc::vec::Vec;
pub use core::hash::Hash; 
pub use core::borrow::Borrow; 

pub struct HashMap<K, V> {
    inner: BTreeMap<K, V>,
}

// TODO: Implement HashMap by ourselves and remove `Ord` constraint.
impl<K: Hash + Ord + Eq, V> HashMap<K, V> {
    pub fn new() -> HashMap<K, V> {
        HashMap {
            inner: BTreeMap::new(),
        }
    }

    pub fn get<Q: ?Sized>(&self, k: &Q) -> Option<&V>
        where K: Borrow<Q>, Q: Hash + Eq + Ord {
        self.inner.get(k)
    }

    pub fn get_mut<Q: ?Sized>(&mut self, k: &Q) -> Option<&mut V>
        where K: Borrow<Q>, Q: Hash + Eq + Ord {
        self.inner.get_mut(k)
    }

    pub fn insert(&mut self, k: K, v: V) -> Option<V> {
        self.inner.insert(k, v)
    }

    pub fn remove<Q: ?Sized>(&mut self, k: &Q) -> Option<V>
        where K: Borrow<Q>, Q: Hash + Eq + Ord {
        self.inner.remove(k)
    }

    pub fn iter<'a>(&'a self) -> Iter<'a, K, V> {
        self.inner.iter()
    }    
}
