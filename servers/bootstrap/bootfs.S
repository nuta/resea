#include <config.h>

.rodata
#ifdef CONFIG_ARCH_ARM64
.balign 4096
#else
.align 4096
#endif

.global __bootfs, __bootfs_end
__bootfs:
    .incbin BOOTFS_PATH
__bootfs_end:
