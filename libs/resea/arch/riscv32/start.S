.align 4
.global start
start:
    mv fp, zero

    ecall
    ecall
    ecall
    ecall
    ecall
1:
    j 1b
