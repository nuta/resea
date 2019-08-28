#include <thread.h>
#include <timer.h>
#include <types.h>

static uintmax_t jiffies = 1;
void timer_interrupt_handler(void) {
    // TODO: Disable UBSan here.
    jiffies++;

    if (jiffies % THREAD_SWITCH_INTERVAL == 0) {
        thread_switch();
    }
}
