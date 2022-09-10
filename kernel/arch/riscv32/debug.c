#include "debug.h"
#include "task.h"
#include <print_macros.h>

static uint32_t *stack_bottom(void) {
    uint32_t sp;
    __asm__ __volatile__("mv %0, sp" : "=r"(sp));
    DBG("sp = %p, bottom_sp = %p", sp, ALIGN_DOWN(sp, KERNEL_STACK_SIZE));
    return (uint32_t *) ALIGN_DOWN(sp, KERNEL_STACK_SIZE);
}

/// Writes the stack canary at the borrom of the current kernel stack.
void stack_reset_current_canary(void) {
    stack_set_canary((uint32_t) stack_bottom());
}

void stack_set_canary(uint32_t sp_bottom) {
    *((uint32_t *) sp_bottom) = STACK_CANARY_VALUE;
}

/// Checks that the kernel stack canary is still alive.
void stack_check(void) {
    if (*stack_bottom() != STACK_CANARY_VALUE) {
        PANIC("the kernel stack has been exhausted");
    }
}
