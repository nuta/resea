#pragma once
#include <types.h>

void memcpy_from_user(void *dst, __user const void *src, size_t len);
void memcpy_to_user(__user void *dst, const void *src, size_t len);
long handle_syscall(int n, long a1, long a2, long a3, long a4, long a5);
