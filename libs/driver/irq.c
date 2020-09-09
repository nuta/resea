#include <resea/syscall.h>
#include <driver/irq.h>

error_t irq_acquire(unsigned irq) {
    return sys_irq_acquire(irq);
}

error_t irq_release(unsigned irq) {
    return sys_irq_release(irq);
}
