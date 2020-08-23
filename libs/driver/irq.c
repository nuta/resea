#include <resea/syscall.h>
#include <driver/irq.h>

error_t irq_acquire(unsigned irq) {
    return sys_listen(-1 /* do nothing */, irq + 1);
}

error_t irq_release(unsigned irq) {
    return sys_listen(-1 /* do nothing */, -(irq + 1));
}
