use core::sync::atomic::{AtomicUsize, Ordering};
use core::fmt;

#[repr(transparent)]
pub struct Stat(AtomicUsize);

impl Stat {
    const fn new() -> Stat {
        Stat(AtomicUsize::new(0))
    }

    #[inline(always)]
    pub fn increment(&self) {
        self.0.fetch_add(1, Ordering::Relaxed);
    }
}

impl fmt::Display for Stat {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.0.load(Ordering::Acquire))
    }
}

pub static IPC_TOTAL: Stat = Stat::new();
pub static IPC_ERRORS: Stat = Stat::new();
pub static IPC_SEND: Stat = Stat::new();
pub static IPC_RECV: Stat = Stat::new();
pub static IPC_KCALLS: Stat = Stat::new();
pub static THREAD_NEW: Stat = Stat::new();
pub static THREAD_SWITCHES: Stat = Stat::new();
pub static PROCESS_NEW: Stat = Stat::new();
pub static CHANNEL_NEW: Stat = Stat::new();
pub static PID_ALLOCS: Stat = Stat::new();
pub static POOL_ALLOCS: Stat = Stat::new();
pub static POOL_FREES: Stat = Stat::new();
pub static TIMER_TICK: Stat = Stat::new();
pub static PAGE_FAULT_TOTAL: Stat = Stat::new();

pub fn print() {
    printk!("kernel statistics --------------------------------");
    printk!("ipc: total={}, send={}, recv={}, kcalls={}, errors={}",
        IPC_TOTAL, IPC_SEND, IPC_RECV, IPC_KCALLS, IPC_ERRORS);
    printk!("process: new={}",
        PROCESS_NEW);
    printk!("channel: new={}",
        CHANNEL_NEW);
    printk!("pid: allocs={}",
        PID_ALLOCS);
    printk!("thread: new={}, switches={}",
        THREAD_NEW, THREAD_SWITCHES);
    printk!("allocator: allocs={}, frees={}",
        POOL_ALLOCS, POOL_FREES);
    printk!("exception: pagefaults={}",
        PAGE_FAULT_TOTAL);
    printk!("timer: ticks={}",
        TIMER_TICK);
}