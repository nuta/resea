#include "debug.h"
#include "task.h"
#include <print_macros.h>

// TODO: move somewhere else
uint32_t *arch_kernel_stack_bottom(void) {
    vaddr_t sp = (vaddr_t) __builtin_frame_address(0);
    return (uint32_t *) ALIGN_DOWN(sp, KERNEL_STACK_SIZE);
}

/// Writes the stack canary at the borrom of the current kernel stack.
void stack_set_canary(void) {
    *arch_kernel_stack_bottom() = STACK_CANARY_VALUE;
}

/// Checks that the kernel stack canary is still alive.
void stack_check(void) {
    if (*arch_kernel_stack_bottom() != STACK_CANARY_VALUE) {
        PANIC("the kernel stack has been exhausted");
    }
}
