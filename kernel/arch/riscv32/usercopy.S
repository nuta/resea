
// void arch_memcpy_from_user(void *dst, __user const void *src, size_t len);
// void arch_memcpy_to_user(__user void *dst, const void *src, size_t len);
.global arch_memcpy_from_user
.global arch_memcpy_to_user
arch_memcpy_from_user:
arch_memcpy_to_user:
