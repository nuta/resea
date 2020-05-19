.rodata
.align 4096
.global __bootfs, __bootfs_end
__bootfs:
    .incbin BOOTFS_PATH
__bootfs_end:
