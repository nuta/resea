#include <resea/malloc.h>
#include <resea/handle.h>
#include <resea/printf.h>
#include <resea/cmdline.h>
#include <string.h>

extern char __cmdline[];
extern char __bss[];
extern char __bss_end[];

// for sparse
__noreturn void resea_init(void);
void main(const char *cmdline);

__noreturn void resea_init(void) {
    memset(__bss, 0, (vaddr_t) __bss_end - (vaddr_t) __bss);
    malloc_init();
    cmdline_init();
    main(__cmdline);
    task_exit();
}
