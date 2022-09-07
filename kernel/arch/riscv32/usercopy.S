
// void arch_memcpy_from_user(void *dst, __user const void *src, size_t len);
.global arch_memcpy_from_user
.global usercopy_point1
arch_memcpy_from_user:
    beqz a2, 2f
1:
usercopy_point1:
    lb a3, 0(a1)

    addi a2, a2, -1
    addi a1, a1, 1
    addi a4, a0, 1
    sb a3, 0(a0)
    mv a0, a4
    bnez a2, 1b
2:
    ret

// void arch_memcpy_to_user(__user void *dst, const void *src, size_t len);
.global arch_memcpy_to_user
.global usercopy_point2
arch_memcpy_to_user:
    beqz a2, 2f
1:
    lb a3, 0(a1)
    addi a2, a2, -1
    addi a1, a1, 1
    addi a4, a0, 1

usercopy_point2:
    sb a3, 0(a0)

    mv a0, a4
    bnez a2, 1b
2:
    ret
