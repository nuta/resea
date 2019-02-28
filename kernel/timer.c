#include <types.h>
#include <timer.h>
#include <thread.h>

static uintmax_t jiffies = 1;
void timer_interrupt_handler(void) {
    jiffies++;

    if (jiffies % THREAD_SWITCH_INTERVAL == 0) {
        thread_switch();
    }
}
