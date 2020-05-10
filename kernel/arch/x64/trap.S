#include <trap.h>
.intel_syntax noprefix

.global syscall_entry
syscall_entry:
    //
    //  Current register values:
    //
    //   RDI: syscall
    //   RSI: arg1
    //   RDX: arg2
    //   RCX: user RIP (set by SYSCALL)
    //   R8:  arg4
    //   R9:  arg5
    //   R10: arg3
    //   R11: user RFLAGS (set by SYSCALL)
    //   The others: user values (need to be preserved)
    //

    // Interrupts are automatically disabled by MSR_FMASK. If interrupts are
    // enabled, an interrupt may occurs before SWAPGS and as a result, an
    // interrupt handler mistakenly use the user's GS base because the interrupt
    // has occurred in the kernel mode and SWAPGS won't be performed.
    //
    // Note that we can't re-enable interrupts until we go back into the user
    // mode or resume the next thread: the (per-thread) kernel stack is shared
    // with exception/interrupt handlers!
    swapgs

    // Switch to the kernel stack.
    mov gs:[GS_RSP3], rsp
    mov rsp, gs:[GS_RSP0]

    test byte ptr gs:[GS_ABI_EMU], 1
    jnz abi_emu_entry

    // Save SYSRET context and callee-saved registers onto the kernel stack.
    push gs:[GS_RSP3] // User RSP.
    push r11 // User RFLAGS.
    push rcx // User RIP.
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov rcx, r10 // arg3
    call x64_handle_syscall

return_to_user:
    // Restore callee-saved registers.
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    pop rcx
    pop r11
    pop rsp

    // Sanitize registers to prevent information leak. Note that some scratch
    // registers should not be zeroed:
    //
    //   EAX: The return value.
    //   RCX: User RIP.
    //   R11: User RFLAGS.
    //
    xor rdx, rdx
    xor rdi, rdi
    xor rsi, rsi
    xor r8, r8
    xor r9, r9
    xor r10, r10

    // Ensure that interrupts are disabled since SWAPGS and SYSRETQ are not a
    // atomic operation. If an interrupt occurred between SWAPGS and SYSRETQ
    // (thus the GS base points to the user one), the interrupt handler won't
    // perform SWAPGS since the interrupt has occurred in kernel mode.
    cli
    swapgs
    sysretq

abi_emu_entry:
    push gs:[GS_RSP3] // User RSP.
    push r15
    push r14
    push r13
    push r12
    push r10
    push r9
    push r8
    push rsi
    push rdi
    push rdx
    push rbx
    push rax
    push rbp
    push r11 // User RFLAGS.
    push rcx // User RIP.

    // User GS base.
    swapgs
    rdgsbase rax
    swapgs
    push rax

    // User FS base.
    rdfsbase rax
    push rax

    mov rdi, rsp
    call x64_abi_emu_hook

    // User FS base.
    pop rax
    wrfsbase rax

    // User GS base.
    pop rax
    swapgs
    wrgsbase rax
    swapgs

    pop rcx // User RIP.
    pop r11 // User RFLAGS.
    pop rbp
    pop rax
    pop rbx
    pop rdx
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r12
    pop r13
    pop r14
    pop r15
    pop rsp

    cli
    swapgs
    sysretq

//
//  Interrupt/exception handlers
//
.align INTERRUPT_HANDLER_SIZE
.global interrupt_handlers
interrupt_handlers:
.set i, 0
.rept 256
.set handler_start, .
// Exceptions with error code.
.if i == 8 || 10 <= i && i <= 14 || i == 17
    .align INTERRUPT_HANDLER_SIZE
    cli
    push i
    jmp interrupt_common
    .align INTERRUPT_HANDLER_SIZE
// Interrupts and exceptions without error code.
.else
    .align INTERRUPT_HANDLER_SIZE
    cli
    push 0 // Dummy value as error code.
    push i
    jmp interrupt_common
    .align INTERRUPT_HANDLER_SIZE
.endif

// Increment the counter.
.set i, i + 1
.endr

.extern x64_handle_interrupt
interrupt_common:
    //
    //  The current stack frame:
    //
    //            +--------------------+
    //     48     |        SS          |
    //            +--------------------+
    //     40     |        RSP         |
    //            +--------------------+
    //     32     |       RFLAGS       |
    //            +--------------------+
    //     24     |        CS          |
    //            +--------------------+
    //     16     |        RIP         |
    //            +--------------------+
    //      8     |     Error code     |
    //            +--------------------+
    //      0     |     IRQ Number     | <- RSP
    //            +--------------------+
    //

    // Check CS register in the IRET frame to determine if the interrupt has
    // occurred in user mode.
    test qword ptr [rsp + 24], 3
    jz 1f
    swapgs
1:
    // Save RDI and set the IRQ number to RDI at once.
    xchg rdi, [rsp]

    // Save registers except RDI (we have already saved it above).
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rsi
    push rdx
    push rcx
    push rbx
    push rax

    mov rsi, rsp
    call x64_handle_interrupt

    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    pop rdi

    // Skip error code.
    add rsp, 8

    // Check CS register in the IRET frame to determine whether the exception
    // occur in the userspace. If so, do SWAPGS.
    test qword ptr [rsp + 8], 3
    jz 1f

    cli
    swapgs
1:
    iretq

.global userland_entry
userland_entry:
    // Set the stack canary in the interrupt stack.
    mov rbp, rsp
    call stack_set_canary

    // Temporarily switch into the syscall stack and set the stack canary.
    mov rbx, rsp
    mov rsp, gs:[GS_RSP0]
    mov rbp, rsp
    call stack_set_canary
    mov rsp, rbx

    test byte ptr gs:[GS_ABI_EMU], 1
    jz 1f

    sub rsp, 18 * 8
    mov rdi, rsp
    mov rsi, 1 // ABI_HOOK_INITIAL
    call x64_abi_emu_hook_initial
    call unlock

    // User FS base.
    pop rax
    wrfsbase rax

    // User GS base.
    pop rax
    swapgs
    wrgsbase rax
    swapgs

    pop rcx // User RIP.
    pop r11 // User RFLAGS.
    pop rbp
    pop rax
    pop rbx
    pop rdx
    pop rdi
    pop rsi
    pop r8
    pop r9
    pop r10
    pop r12
    pop r13
    pop r14
    pop r15

    mov [rsp + 8], rcx  // RIP in a IRET frame
    mov [rsp + 24], r11 // RFLAGS in a IRET frame
    pop rcx
    mov [rsp + 24], rcx // RSP in a IRET frame
    mov rcx, [rsp + 0]

    jmp 2f
1:
    call unlock

    // Sanitize registers to prevent information leak.
    xor rax, rax
    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx
    xor rdi, rdi
    xor rsi, rsi
    xor r8, r8
    xor r9, r9
    xor r10, r10
    xor r11, r11
    xor r12, r12
    xor r13, r13
    xor r14, r14
    xor r15, r15
    xor rbp, rbp

2:
    cli
    swapgs
    iretq

/// void switch_context(uint64_t *prev_rsp, uint64_t *next_rsp);
///
/// Saves the current context into `prev` and restore the context from `next`.
///
/// ``arch'' is a state of registers including:
///
///   - Instruction Pointer (implicitly saved and restored by CALL/RET)
///   - Stack Pointer (RSP)
///   - callee-saved registers (RBP, RBX, R12-R15)
///   - RFLAGS
///
.global switch_context
switch_context:
    /* Save callee-saved registers. */
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15
    pushfq

    /* Save and restore the stack pointer. */
    mov [rdi], rsp
    mov rsp, [rsi]

    /* Restore callee-saved registers. */
    popfq
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp

    /* Resume the next thread. */
    ret

/// void arch_memcpy_from_user(void *dst, const void *src, size_t len);
/// void arch_memcpy_to_user(void *dst, const void *src, size_t len);
///
/// Copies a memory buffer from/to the user space. We don't check the validity
/// of the user's addresses, instead, we handles a page fault occurred at
/// `usercopy1` as a user's fault.
///
/// Note that caller MUST check that the memory range does not overlaps with the
/// kernel sapce!
.global arch_memcpy_from_user, arch_memcpy_to_user, usercopy1
arch_memcpy_from_user:
arch_memcpy_to_user:
    mov rcx, rdx
    cld
usercopy1:
    rep movsb
    ret

/// arch_strncpy_from_user(char *dst, vaddr_t src, size_t max_len);
///
/// Copies a NUL-terminated string from the user space. Unlike traditional
/// strncpy, if max_len > 0, this function always set a NUL character at the end
/// of the destination buffer even if max_len is less than or equals to the
/// length of the source string.
///
/// We don't check the validity of the user's addresses, instead, we handles a
/// page fault occurred at `usercopy2` as a user's fault.
///
/// Note that caller MUST check that the memory range does not overlaps with the
/// kernel sapce!
.global arch_strncpy_from_user, usercopy2
arch_strncpy_from_user:
    // Check if max_len > 0.
    test rdx, rdx
    jz 3f

    mov rcx, rdx
1:
    // Does dst has more room?
    test rcx, rcx
    jz 2f

usercopy2:
    mov al, [rsi]
    mov [rdi], al
    add rdi, 1

    // Is a NUL character?
    test al, al
    jz 2f

    // Continue copying.
    add rsi, 1
    sub rcx, 1
    jmp 1b

2:
    // Set a NUL character at the end of dst.
    mov byte ptr [rdi - 1], 0
3:
    ret
