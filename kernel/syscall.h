#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>

/// A pointer given by the user. Don't reference it directly; access it through
/// safe functions such as memcpy_from_user and memcpy_to_user!
typedef vaddr_t userptr_t;

void memcpy_from_user(void *dst, userptr_t src, size_t len);
void memcpy_to_user(userptr_t dst, const void *src, size_t len);
uintmax_t handle_syscall(uintmax_t syscall, uintmax_t arg1, uintmax_t arg2,
                         uintmax_t arg3, uintmax_t arg4, uintmax_t arg5);

// Implemented in arch.
struct task;
void arch_memcpy_from_user(void *dst, userptr_t src, size_t len);
void arch_memcpy_to_user(userptr_t dst, const void *src, size_t len);
void arch_strncpy_from_user(char *dst, userptr_t src, size_t max_len);

#endif
