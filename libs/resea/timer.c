#include <resea/syscall.h>
#include <resea/timer.h>

// TODO: Support multiple timers.

error_t timer_set(msec_t timeout) {
    return sys_timer_set(timeout);
}
