.intel_syntax noprefix
.text

.global start
start:
    // Clear .bss section
    mov al, 0x00
    lea rdi, [rip + __bss]
    lea rcx, [rip + __bss_end]
    sub rcx, rdi
    cld
    rep stosb

    // Set the stack pointer.
    lea rsp, [rip + __stack_end]

    // Set RBP to 0 in order to stop backtracing here.
    mov rbp, 0

    // Initialize the user library.
    call resea_init

    // Call main().
    call main

    // main() has returned. Exit the current task.
    call task_exit

    // Somehow task_exit returned!
.global halt
halt:
    int 3
    jmp halt

//
//  The bootfs header (for init.bin).
//
.section .bootfs, "ax"
.global __bootfs
__bootfs:
    // The startup code (16 bytes). Kernel starts the first userspace program here.
    jmp start
    .align 16

    // Fields to be filled by tools/mkbootfs.py.
    .long 0      // version
    .long 0      // offset of the first file
    .long 0      // number of files
    .long 0      // reserved
