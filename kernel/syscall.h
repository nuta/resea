#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <types.h>
#include <message.h>

/// An attribute for a pointer given by the user. Don't dereference it directly:
/// access it through safe functions such as memcpy_from_user and memcpy_to_user!
#define __user   __attribute__((noderef, address_space(1)))

void memcpy_from_user(void *dst, __user const void *src, size_t len);
void memcpy_to_user(__user void *dst, const void *src, size_t len);
long handle_syscall(int n, long a1, long a2, long a3, long a4, long a5);

#ifdef CONFIG_ABI_EMU
void abi_emu_hook(trap_frame_t *frame, enum abi_hook_type type);
#endif

// Implemented in arch.
void arch_memcpy_from_user(void *dst, __user const void *src, size_t len);
void arch_memcpy_to_user(__user void *dst, const void *src, size_t len);

#endif
