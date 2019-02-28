#include <arch.h>
#include <x64/spinlock.h>
#include <x64/asm.h>

void spin_lock_init(spinlock_t *lock) {
    __atomic_store_n(lock, 0, __ATOMIC_SEQ_CST);
}

void spin_lock(spinlock_t *lock) {
    while(!atomic_compare_and_swap(lock, 0, 1)) {
        asm_pause();
    }
}

void spin_unlock(spinlock_t *lock) {
    __atomic_store_n(lock, 0, __ATOMIC_SEQ_CST);
}

flags_t spin_lock_irqsave(spinlock_t *lock) {
    flags_t flags = asm_read_rflags();
    asm_cli();

    spin_lock(lock);
    return flags;
}

void spin_unlock_irqrestore(spinlock_t *lock, flags_t flags) {
    spin_unlock(lock);
    asm_write_rflags(flags);
}
