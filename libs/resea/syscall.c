#include <resea/malloc.h>
#include <resea/printf.h>
#include <resea/syscall.h>

void nop_syscall(void) {
    syscall(SYS_NOP, 0, 0, 0, 0, 0);
}
