
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

// void arch_strncpy_from_user(char *dst, userptr_t src, size_t max_len);
.global arch_strncpy_from_user, arm64_usercopy3
arch_strncpy_from_user:
    cbz  x2, 3f // check if max_len > 0 (there's a room for '\0')
1:
    cmp  x2, 1
    ble  2f
arm64_usercopy3:
    ldrb w4, [x1], #1
    cbz  w4, 2f
    strb w4, [x0], #1
    sub  x2, x2, 1
    b    1b

2:
    // Append '\0'.
    mov   w4, 0
    strb  w4, [x0]
3:
    ret

