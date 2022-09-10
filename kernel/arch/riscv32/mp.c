#include "mp.h"
#include <kernel/arch.h>
#include <kernel/printk.h>

static int big_lock = UNLOCKED;
static int lock_owner = NO_LOCK_OWNER;

int mp_self(void) {
    return CPUVAR->id;
}

void mp_lock(void) {
    if (mp_self() == lock_owner) {
        PANIC("recursive lock (#%d)", mp_self());
    }

    while (!__sync_bool_compare_and_swap(&big_lock, UNLOCKED, LOCKED)) {}
    lock_owner = mp_self();
}

void panic_lock(void) {
    lock_owner = mp_self();
}

void mp_unlock(void) {
    DEBUG_ASSERT(lock_owner == mp_self());
    lock_owner = NO_LOCK_OWNER;
    __sync_bool_compare_and_swap(&big_lock, LOCKED, UNLOCKED);
}

__noreturn void halt(void) {
    for (;;)
        __asm__ __volatile__("wfi");
}
