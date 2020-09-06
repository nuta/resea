#include <task.h>

struct cpuvar cpuvar;

void halt(void) {
    while (true) {
        __asm__ __volatile__("wfi");
    }
}

void mp_start(void) {
    // Do nothing: we don't support multiprocessors.
}

void mp_reschedule(void) {
    // Do nothing: we don't support multiprocessors.
}

void lock(void) {
    // Do nothing: we don't support multiprocessors.
}

void panic_lock(void) {
    // Do nothing: we don't support multiprocessors.
}

void unlock(void) {
    // Do nothing: we don't support multiprocessors.
}

void panic_unlock(void) {
    // Do nothing: we don't support multiprocessors.
}
