#include "syscall.h"
#include "arch.h"
#include "printk.h"

void memcpy_from_user(void *dst, __user const void *src, size_t len) {
    arch_memcpy_from_user(dst, src, len);
}

void memcpy_to_user(__user void *dst, const void *src, size_t len) {
    arch_memcpy_to_user(dst, src, len);
}
