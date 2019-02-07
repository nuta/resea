use crate::thread;
use crate::stats;
use core::sync::atomic::Ordering;
use core::sync::atomic::AtomicUsize;

/// The timer must fire timer_interrupt_handler() every 1/TICK_HZ second.
const TICK_HZ: usize = 1000;
const THREAD_SWITCH_INTERVAL: usize = (50 * 1000) / TICK_HZ;
static JIFFIES: AtomicUsize = AtomicUsize::new(0);

pub fn timer_interrupt_handler() {
    stats::TIMER_TICK.increment();

    let tick = JIFFIES.fetch_add(1, Ordering::SeqCst);
    if tick > 0 && tick % THREAD_SWITCH_INTERVAL == 0 {
        thread::switch();
    }
}