#include <resea/malloc.h>
#include <resea/handle.h>
#include <resea/printf.h>
#include <string.h>

extern char __bss[];
extern char __bss_end[];

void resea_init(void) {
    memset(__bss, 0, (vaddr_t) __bss_end - (vaddr_t) __bss);
    malloc_init();
}
