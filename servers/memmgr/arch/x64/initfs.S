.set MEMMGR_STACK_END, 0x03400000

.intel_syntax noprefix
.section .initfs, "ax"

.global __initfs
__initfs:
    jmp start_memmgr // startup code
    .align 16

    // Fields to be filled by tools/mkinitfs.py.
    .long 0      // version
    .long 0      // size
    .long 0      // number of files

    // The initfs_file entry for "memmgr.bin".
    .align 256

    // struct bootinfo
.global __bootinfo
__bootinfo:
    .space 1024

.section .text
.global start_memmgr
start_memmgr:
    mov rsp, MEMMGR_STACK_END
    jmp start
