.intel_syntax noprefix
.text

.global start
start:
    // Set the stack pointer.
    lea rsp, [rip + __stack_end]

    // Set RBP to 0 in order to stop backtracing here.
    mov rbp, 0

    // Initialize the user library.
    call resea_init

    // main() has returned. Exit the current task.
    call task_exit

    // Somehow task_exit returned!
.global halt
halt:
    int 3
    jmp halt

.section .bootelf_header, "ax"
.global __bootelf_header, __bootinfo
__bootelf_header:
    .ascii "00BOOT\xe1\xff" // magic
    .space 16               // name
    .quad  0                // uint64_t entry
    .space 128              // mappings
__bootinfo:
    // struct bootinfo bootinfo;
    .space 512
