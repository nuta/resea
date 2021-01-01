#include <printk.h>
#include <types.h>

void arch_printchar(char ch) {
}

char kdebug_readchar(void) {
    return '\0';
}

bool kdebug_is_readable(void) {
    return false;
}
