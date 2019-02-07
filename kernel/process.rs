use crate::allocator::{Allocator, Ref, OBJ_POOL};
use crate::arch::PageTable;
use crate::memory::VmArea;
use crate::channel::{ChannelTable, Channel};
use crate::collections::{List, Link};
use crate::thread::Thread;
use crate::utils::LazyStatic;
use crate::stats;
use crate::printk;
use core::sync::atomic::{AtomicIsize, Ordering};

#[derive(Copy, Clone, PartialEq)]
#[repr(transparent)]
pub struct PId(isize);

impl PId {
    pub fn from_isize(id: isize) -> PId {
        PId(id)
    }

    pub fn as_isize(&self) -> isize {
        self.0
    }
}

impl<'a> printk::Displayable<'a> for PId {
    fn display(&self) -> printk::Argument<'a> {
        printk::Argument::Signed(self.0 as isize)
    }
}

// The PID starts with 2. The ID 0 is reserved for the idle thread and 1 is for
// the kernel process.
static NEXT_PID: AtomicIsize = AtomicIsize::new(2);
pub fn alloc_pid() -> PId {
    let pid = NEXT_PID.fetch_add(1, Ordering::SeqCst);

    stats::PID_ALLOCS.increment();
    PId::from_isize(pid)
}

/// Pager.
pub enum Pager {
    UserPager(Ref<Channel>),
    InitfsPager,
    StraightMappingPager,
}

/// Process.
pub struct Process {
    /// The process ID.
    pid: PId,
    /// Regions in virtual memory space.
    vmareas: List<Ref<VmArea>>,
    /// The page table, a mapping table between virtual address and physical
    /// address. It is fully architecture-dependent.
    page_table: PageTable,
    /// Threads belongs to the process.
    threads: List<Ref<Thread>>,
    /// Channels.
    channels: ChannelTable,
    next: Link<Ref<Process>>,
}

static PROCESS_ALLOCATOR: LazyStatic<Allocator<'static, Process>> = LazyStatic::new();

impl Process {
    /// Allocates a new process object.
    pub fn new() -> Ref<Process> {
        let pid = alloc_pid();
        Process::new_with_pid(pid)
    }

    /// Allocates a new process object with given pid.
    fn new_with_pid(pid: PId) -> Ref<Process> {
        let process = PROCESS_ALLOCATOR.alloc_init_or_panic2(|proc| Process {
            pid,
            vmareas: list_new!(VmArea, next),
            page_table: PageTable::new(),
            threads: list_new!(Thread, process_link),
            channels: ChannelTable::new(proc),
            next: Link::null(),
        });

        PROCESSES.push_back(process.clone());
        stats::PROCESS_NEW.increment();
        process
    }

    #[inline]
    pub fn pid(&self) -> PId {
        self.pid
    }

    #[inline]
    pub fn page_table(&self) -> &PageTable {
        &self.page_table
    }

    #[inline]
    pub fn vmareas(&self) -> &List<Ref<VmArea>> {
        &self.vmareas
    }

    #[inline]
    pub fn channels(&self) -> &ChannelTable {
        &self.channels
    }

    pub fn add_vmarea(&self, vmarea: Ref<VmArea>) {
        self.vmareas.push_back(vmarea);
    }

    pub fn add_thread(&self, thread: Ref<Thread>) {
        self.threads.push_back(thread);
    }
}

static PROCESSES: LazyStatic<List<Ref<Process>>> = LazyStatic::new();
pub fn get_process_by_pid(pid: &PId) -> Option<Ref<Process>> {
    for proc in PROCESSES.iter() {
        if proc.pid() == *pid {
            return Some(proc.clone());
        }
    }

    None
}


pub static KERNEL_PROCESS: LazyStatic<Ref<Process>> = LazyStatic::new();

pub fn init() {
    PROCESS_ALLOCATOR.init(Allocator::new(&OBJ_POOL));
    PROCESSES.init(list_new!(Process, next));
    KERNEL_PROCESS.init(Process::new_with_pid(PId::from_isize(1)))
}
