#include <resea.h>

NORETURN void unreachable(void) {
    __asm__ __volatile__("ud2");
    __builtin_unreachable();
}
