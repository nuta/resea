#include <resea/timer.h>
#include <resea/syscall.h>

// TODO: Support multiple timers.

error_t timer_set(msec_t timeout) {
    return sys_listen(timeout, 0 /* do nothing */);
}
