#include <task.h>
#include "asm.h"

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
    // TODO:
}

void panic_lock(void) {
    // TODO:
}

void unlock(void) {
    // TODO:
}

void panic_unlock(void) {
    // TODO:
}
