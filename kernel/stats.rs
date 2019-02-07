use core::sync::atomic::{AtomicUsize, Ordering};
use crate::printk;

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

impl<'a> printk::Displayable<'a> for Stat {
    fn display(&self) -> printk::Argument<'a> {
        printk::Argument::Unsigned(self.0.load(Ordering::SeqCst))
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
    printk!("ipc: total=%d, send=%d, recv=%d, kcalls=%d, errors=%d",
        IPC_TOTAL, IPC_SEND, IPC_RECV, IPC_KCALLS, IPC_ERRORS);
    printk!("process: new=%d", PROCESS_NEW);
    printk!("channel: new=%d", CHANNEL_NEW);
    printk!("pid: allocs=%d", PID_ALLOCS);
    printk!("thread: new=%d, switches=%d", THREAD_NEW, THREAD_SWITCHES);
    printk!("allocator: allocs=%d, frees=%d", POOL_ALLOCS, POOL_FREES);
    printk!("exception: pagefaults=%d", PAGE_FAULT_TOTAL);
    printk!("timer: ticks=%d", TIMER_TICK);
}