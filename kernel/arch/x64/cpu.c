#include <arch.h>
#include <x64/cpu.h>

void arch_panic(void) {
    for (;;) {
        __asm__ __volatile__("cli; hlt");
    }
}

void arch_idle(void) {

}
