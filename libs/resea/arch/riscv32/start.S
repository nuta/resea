.align 4
.global start
start:
    mv fp, zero
    mv ra, zero
    la sp, __stack
    jal resea_init
    jal main
1:
    j 1b
