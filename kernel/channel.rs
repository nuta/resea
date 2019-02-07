use crate::process::Process;
use crate::thread::Thread;
use crate::allocator::{Ref, Allocator, PAGE_POOL, OBJ_POOL};
use crate::utils::{LazyStatic, compare_and_swap};
use crate::arch::PAGE_SIZE;
use crate::collections::List;
use crate::printk;
use crate::stats;
use core::sync::atomic::{AtomicPtr, AtomicUsize};
use core::mem::size_of;
use core::cell::Cell;
use core::ptr::null_mut;
use core::sync::atomic::Ordering;

#[repr(transparent)]
#[derive(Clone, Copy, Debug)]
pub struct CId(isize);

impl CId {
    pub fn from_isize(id: isize) -> CId {
        CId(id)
    }

    pub fn as_isize(&self) -> isize {
        self.0
    }
}

impl<'a> printk::Displayable<'a> for CId {
    fn display(&self) -> printk::Argument<'a> {
        printk::Argument::Signed(self.0 as isize)
    }
}

pub struct Channel {
    cid: CId,
    process: Ref<Process>,
    ref_count: AtomicUsize,
    linked_to: Cell<Ref<Channel>>,
    transfer_to: Cell<Ref<Channel>>,
    receiver: AtomicPtr<Thread>,
    sender_queue: List<Ref<Thread>>,
}

impl Channel {
    pub fn cid(&self) -> CId {
        self.cid
    }

    pub fn process(&self) -> Ref<Process> {
        self.process
    }

    pub fn linked_to(&self) -> Ref<Channel> {
        self.linked_to.get()
    }

    pub fn transfer_to(&self) -> Ref<Channel> {
        self.transfer_to.get()
    }

    pub fn receiver(&self) -> &AtomicPtr<Thread> {
        &self.receiver
    }

    pub fn sender_queue(&self) -> &List<Ref<Thread>> {
        &self.sender_queue
    }
}

pub fn link(ch1: Ref<Channel>, ch2: Ref<Channel>) {
    ch1.linked_to.set(ch2);
    ch2.linked_to.set(ch1);
    ch1.ref_count.fetch_add(1, Ordering::SeqCst);
    ch2.ref_count.fetch_add(1, Ordering::SeqCst);
}

pub fn transfer(src: Ref<Channel>, dst: Ref<Channel>) {
    src.transfer_to.set(dst);
    dst.ref_count.fetch_add(1, Ordering::SeqCst);
}

pub struct ChannelTable {
    table: &'static [*mut Channel],
    process: Ref<Process>,
}

static CHANNEL_ALLOCATOR: LazyStatic<Allocator<'static, Channel>> = LazyStatic::new();

impl ChannelTable {
    pub fn new(process: Ref<Process>) -> ChannelTable {
        let num = PAGE_SIZE / size_of::<*mut Channel>();
        let table = unsafe {
            let page = PAGE_POOL.alloc_or_panic().as_ptr();
            core::slice::from_raw_parts(page, num)
        };

        ChannelTable {
            table,
            process,
        }
    }

    pub fn alloc(&self) -> Option<Ref<Channel>> {
        for i in 0..self.table.len() {
            let ch = CHANNEL_ALLOCATOR.alloc_init_or_panic2(|ch| Channel {
                cid: CId::from_isize(i as isize + 1),
                process: self.process,
                ref_count: AtomicUsize::new(1),
                linked_to: Cell::new(ch),
                transfer_to: Cell::new(ch),
                receiver: AtomicPtr::new(null_mut()),
                sender_queue: list_new!(Thread, sender_queue_link),
            });

            // Try to reserve the channel ID: i + 1.
            unsafe {
                let table_ptr = self.table.as_ptr() as *mut *mut Channel;
                let ptr = table_ptr.offset(i as isize);
                if compare_and_swap(ptr as *mut usize, 0, ch.as_ptr() as usize) {
                    // Successfully swapped the pointer in the table at `i',
                    // namely, we have registered the channel ID `i + 1`.
                    stats::CHANNEL_NEW.increment();
                    return Some(ch);
                }
            }

            CHANNEL_ALLOCATOR.free(ch);
        }

        None
    }

    pub fn get(&self, cid: CId) -> Option<Ref<Channel>> {
        if cid.as_isize() == 0 {
            // The cid is out of bounds.
            return None;
        }

        let index = cid.as_isize() as usize - 1;
        if index >= self.table.len() {
            // The cid is out of bounds.
            return None;
        }

        if let Some(ch) = self.table.get(index) {
            if *ch == core::ptr::null_mut() {
                // The cid is not in use.
                None
            } else {
                Some(Ref::from_ptr(*ch))
            }
        } else {
            None
        }
    }
}

pub fn init() {
    CHANNEL_ALLOCATOR.init(Allocator::new(&OBJ_POOL));
}
