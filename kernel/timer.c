#include <thread.h>
#include <timer.h>
#include <types.h>

static uintmax_t jiffies = 1;

/// The timer interrupt handler.
void timer_interrupt_handler(void) {
    // TODO: Disable UBSan here. Overflows is intended behavior.
    jiffies++;

    if (jiffies % THREAD_SWITCH_INTERVAL == 0) {
        thread_switch();
    }
}
