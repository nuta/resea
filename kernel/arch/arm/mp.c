#include <task.h>

struct cpuvar cpuvar;

void halt(void) {
    __asm__ __volatile__("wfi");
}

void panic_lock(void) {
    // Do nothing: we don't support multiprocessors.
}

void mp_start(void) {
    // Do nothing: we don't support multiprocessors.
}

void mp_reschedule(void) {
    // Do nothing: we don't support multiprocessors.
}
