#include <resea/timer.h>
#include <resea/syscall.h>

error_t timer_set(msec_t timeout) {
    return sys_setattrs(NULL, 0, timeout);
}
