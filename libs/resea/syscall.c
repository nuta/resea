#include <resea/syscall.h>

void sys_console_write(const char *buf, size_t len) {
    register int32_t a0 __asm__("a0") = 0;
    register int32_t a1 __asm__("a1") = (uint32_t) buf;
    register int32_t a2 __asm__("a2") = (uint32_t) len;
    register int32_t result __asm__("a0");
    __asm__ __volatile__("ecall"
                         : "=r"(result)
                         : "r"(a0), "r"(a1), "r"(a2)
                         : "memory");
}
