
// void arch_memcpy_from_user(void *dst, userptr_t src, size_t len);
.global arch_memcpy_from_user, arm64_usercopy1
arch_memcpy_from_user:
1:
    cbz  x2, 2f
arm64_usercopy1:
    ldrb w4, [x1], #1
    strb w4, [x0], #1
    sub  x2, x2, 1
    b    1b
2:
    ret

// void arch_memcpy_to_user(userptr_t dst, const void *src, size_t len);
.global arch_memcpy_to_user, arm64_usercopy2
arch_memcpy_to_user:
1:
    cbz  x2, 2f
    ldrb w4, [x1], #1
arm64_usercopy2:
    strb w4, [x0], #1
    sub  x2, x2, 1
    b    1b
2:
    ret
