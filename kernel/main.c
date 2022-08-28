#include <kernel/main.h>
#include <kernel/arch.h>

void kernel_main(void) {
    arch_console_write("Hello, world!\n");
}
