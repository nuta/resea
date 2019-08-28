#include "memory_map.h"
.intel_syntax noprefix
.section .initfs, "ax"

.globl __initfs
__initfs:
    jmp start_memmgr // startup code
    .align 16

    // Fields to be filled by tools/mkinitfs.py.
    .long 0      // version
    .long 0      // size
    .long 0      // number of files

    // The initfs_file entry for "memmgr.bin".
    .space 256

.section .text
start_memmgr:
    mov rsp, MEMMGR_STACK_END
    jmp start
