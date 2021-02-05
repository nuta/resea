#include <resea/cmdline.h>
#include <resea/handle.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/syscall.h>
#include <string.h>

extern char __cmdline[];
extern char __bss[];
extern char __bss_end[];

// for sparse
__noreturn void resea_init(void);
void main(const char *cmdline);

int foobar();

__noreturn void resea_init(void) {
    sys_console_write("HI1\n", 4);
    foobar();
    // memset(__bss, 0, (vaddr_t) __bss_end - (vaddr_t) __bss);
    sys_console_write("HI2\n", 4);
    // malloc_init();
    // cmdline_init();
    // main(__cmdline);
    // task_exit();
}
