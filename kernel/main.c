#include <kernel/main.h>
#include <kernel/arch.h>

void kernel_main(void) {
    arch_console_write('H');
    arch_console_write('e');
    arch_console_write('l');
    arch_console_write('l');
    arch_console_write('o');
    arch_console_write('\n');
}
