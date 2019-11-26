#include <arch.h>
#include <thread.h>
#include <support/printk.h>
#include <support/stack_protector.h>

#define STACK_CANARY 0xdeadca71 /* dead canary */

/// Returns the pointer to the kernel stack's canary. The stack canary exists
/// at the end (bottom) of the stack. The kernel stack should be large enough
/// in order not to modify the canry value. This function assumes that the size
/// of kernel stack equals to PAGE_SIZE.
static inline vaddr_t get_current_stack_canary_address(void) {
    STATIC_ASSERT(PAGE_SIZE == KERNEL_STACK_SIZE);
    return ALIGN_DOWN(arch_get_stack_pointer(), PAGE_SIZE);
}

/// Writes a kernel stack protection marker. The value `STACK_CANARY` is
/// verified by calling check_stack_canary().
void init_stack_canary(vaddr_t stack_bottom) {
    *((uint32_t *) stack_bottom) = STACK_CANARY;
}

/// Verifies that stack canary is alive. If not so, register a complaint.
void check_stack_canary(void) {
    uint32_t *canary = (uint32_t *) get_current_stack_canary_address();
    if (*canary != STACK_CANARY) {
        PANIC("The stack canary is no more! This is an ex-canary!");
    }
}

/// Each CPU have to call this function once during the boot.
void init_boot_stack_canary(void) {
    init_stack_canary(get_current_stack_canary_address());
}
