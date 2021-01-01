#include "asm.h"
#include <machine/machine.h>
#include <task.h>

static struct cpuvar cpuvars[NUM_CPUS_MAX];

struct cpuvar *arm64_get_cpuvar(void) {
    return &cpuvars[mp_self()];
}

void halt(void) {
    while (true) {
        __asm__ __volatile__("wfi");
    }
}

void mp_start(void) {
    machine_mp_start();
}

void mp_reschedule(void) {
    // TODO:
}

#define LOCKED        0x12ab
#define UNLOCKED      0xc0be
#define NO_LOCK_OWNER -1
static int big_lock = UNLOCKED;
static int lock_owner = NO_LOCK_OWNER;

void lock(void) {
    return;  // FIXME:

    if (mp_self() == lock_owner) {
        PANIC("recursive lock (#%d)", mp_self());
    }

    while (!__sync_bool_compare_and_swap(&big_lock, UNLOCKED, LOCKED)) {
        //        __asm__ __volatile__("");
    }

    lock_owner = mp_self();
}

void panic_lock(void) {
    lock_owner = mp_self();
}

void unlock(void) {
    return;  // FIXME:

    DEBUG_ASSERT(lock_owner == mp_self());
    lock_owner = NO_LOCK_OWNER;
    __sync_bool_compare_and_swap(&big_lock, LOCKED, UNLOCKED);
}

void panic_unlock(void) {
    if (mp_self() == lock_owner) {
        lock_owner = NO_LOCK_OWNER;
    }
}
