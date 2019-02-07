use crate::allocator::{Allocator, Ref, OBJ_POOL};
use crate::arch::{self, cpu, current, set_current_thread, ArchThread};
use crate::collections::{Link, List};
use crate::process::{alloc_pid, Process, KERNEL_PROCESS};
use crate::utils::{VAddr, LazyStatic};
use crate::stats;
use crate::printk;
use core::cell::Cell;

#[derive(Clone, Copy, Debug, PartialEq)]
#[repr(transparent)]
pub struct TId(isize);

impl TId {
    pub fn from_isize(id: isize) -> TId {
        TId(id)
    }

    pub fn as_isize(&self) -> isize {
        self.0
    }
}

impl<'a> printk::Displayable<'a> for TId {
    fn display(&self) -> printk::Argument<'a> {
        printk::Argument::Signed(self.0 as isize)
    }
}

pub fn alloc_tid() -> TId {
    TId::from_isize(alloc_pid().as_isize())
}

#[derive(Clone, Copy, PartialEq, Eq)]
pub enum ThreadState {
    /// Ready to run.
    Runnable,
    /// Waiting for a message.
    Blocked,
}

/// Thread. Use `#[repr(C, packed)]` as `arch` assumes that `arch` is the first
/// field and there are no padding.
#[repr(C, packed)]
pub struct Thread {
    /// Architecture-dependent context such as registers. *This field must
    /// come first*.
    arch: ArchThread,
    /// The process.
    process: Ref<Process>,
    /// The thread ID.
    tid: TId,
    /// The current state of the thread.
    state: Cell<ThreadState>,
    /// A link of a linked list in a runqueue.
    runqueue_link: Link<Ref<Thread>>,
    /// A link of a linked list of the process.
    pub process_link: Link<Ref<Thread>>,
    /// A link of a linked list in a sender queue.
    pub sender_queue_link: Link<Ref<Thread>>,
}

static THREAD_ALLOCATOR: LazyStatic<Allocator<'static, Thread>> = LazyStatic::new();

impl Thread {
    /// Allocates a thread object.
    pub fn new(process: Ref<Process>, start: VAddr, stack: VAddr, arg: usize) -> Ref<Thread> {
        let tid = alloc_tid();
        Thread::new_with_tid(process, tid, start, stack, arg)
    }

    /// Allocates a thread object with given TID.
    fn new_with_tid(
        process: Ref<Process>,
        tid: TId,
        start: VAddr,
        stack: VAddr,
        arg: usize,
    ) -> Ref<Thread> {
        let thread = THREAD_ALLOCATOR.alloc_init_or_panic(Thread {
            tid,
            process,
            state: Cell::new(ThreadState::Blocked),
            arch: ArchThread::new(process, start, stack, arg),
            runqueue_link: Link::null(),
            process_link: Link::null(),
            sender_queue_link: Link::null(),
        });

        stats::THREAD_NEW.increment();
        process.add_thread(thread);
        thread
    }

    #[inline(always)]
    pub fn arch(&self) -> &ArchThread {
        &self.arch
    }

    #[inline(always)]
    pub fn process(&self) -> Ref<Process> {
        self.process
    }

    #[inline(always)]
    pub fn tid(&self) -> TId {
        self.tid
    }

    #[inline(always)]
    pub fn state(&self) -> ThreadState {
        self.state.get()
    }

    #[inline(always)]
    pub fn set_state(&self, new_state: ThreadState) {
        self.state.set(new_state)
    }
}

/// Kills the current thread and resume another thread. Never returns.
pub fn kill_current() -> ! {
    // TODO:
    unimplemented!();
}

/// The runqueue. Threads ready to run except the currently running ones
/// are linked to this queue.
static RUN_QUEUE: List<Ref<Thread>> = list_new!(Thread, runqueue_link);

/// Picks a thread to run next. Returns the idle thread if there're no threads
/// in the runqueue.
fn scheduler() -> Ref<Thread> {
    RUN_QUEUE.pop_front().unwrap_or_else(|| cpu().idle_thread.clone())
}

pub fn do_switch(save_current: bool) {
    if current().state() == ThreadState::Runnable && RUN_QUEUE.len() == 0 {
        // No runnable threads other than the current one. Continue executing
        // the current thread.
        return;
    }

    let prev = current();
    let next = scheduler();
    if prev.state() == ThreadState::Runnable {
        // The current thread has spent its quantum. Enqueue it into the
        // runqueue again to resume later.
        enqueue(prev);
    }

    stats::THREAD_SWITCHES.increment();
    arch::switch(prev, next, save_current);
}

/// Resumes another thread.
pub fn switch() {
    do_switch(true);
}

/// Enqueues a thread into the runqueue.
pub fn enqueue(thread: Ref<Thread>) {
    RUN_QUEUE.push_back(thread);
}

pub fn init() {
    THREAD_ALLOCATOR.init(Allocator::new(&OBJ_POOL));

    let idle_thread = Thread::new_with_tid(
        *KERNEL_PROCESS,
        TId::from_isize(0),
        VAddr::null(),
        VAddr::null(),
        0,
    );

    set_current_thread(idle_thread);
    cpu().idle_thread.init(idle_thread)
}
