.set ARCH_THREAD_RSP0, 16
.set USER_RPL, 3
.set USER_DS, 32 | USER_RPL
.intel_syntax noprefix

.align 8
syscall_table:
    .quad x64_ipc_handler         /* 0 */
    .quad invalid_syscall_handler /* 1 */
    .quad x64_open_handler        /* 2 */
    .quad invalid_syscall_handler /* 3 */
    .quad x64_link_handler        /* 4 */
    .quad x64_transfer_handler    /* 5 */
    .quad invalid_syscall_handler /* 6 */
    .quad invalid_syscall_handler /* 7 */
    .quad invalid_syscall_handler /* 8 */
    .quad invalid_syscall_handler /* 9 */
    .quad invalid_syscall_handler /* 11 */
    .quad invalid_syscall_handler /* 12 */
    .quad invalid_syscall_handler /* 13 */
    .quad invalid_syscall_handler /* 14 */
    .quad invalid_syscall_handler /* 15 */

syscall_table_addr:
    .quad syscall_table

.globl x64_syscall_handler
x64_syscall_handler:
    /*
     *  RAX: syscall number
     *  RDI: 1st argument
     *  RSI: 2nd argument
     *  RDX: 3rd argument
     *  RCX: user RIP
     *  R8:  5th arugment
     *  R9:  6th arugment
     *  R10: 4th arugment
     *  R11: user RFLAGS
     *
     */
    cli // FIXME: remove this
    /* Compute the handler address. */
    and rax, 0x0f
    shl rax, 3
    add rax, [rip + syscall_table_addr]
    jmp [rax]

.macro SYSCALL_ENTER
    /* Overwrite SS register since SYSCALL set KERNEL_CS + 8 but we prefer
     * the null descriptor, i.e., SS = 0. */
    xor ax, ax
    mov ss, ax

    /* Switch to the kernel stack. */
    mov rax, rsp
    mov rsp, gs:[ARCH_THREAD_RSP0]

    /* Save SYSRET context. */
    push rax /* user RSP */
    push r11 /* user RFLAGS */
    push rcx /* user RIP */

    /* Set 4th argument. */
    mov rcx, r10
.endm

.macro SYSCALL_LEAVE
    /* Return to the user thread. */
    pop rcx
    pop r11
    pop rsp

    sysretq
.endm

.extern ipc_handler
x64_ipc_handler:
    SYSCALL_ENTER
    call ipc_handler

    /* Set 3rd received payload. */
    mov r10, rcx
    SYSCALL_LEAVE

.extern open_handler
x64_open_handler:
    SYSCALL_ENTER
    call open_handler
    SYSCALL_LEAVE

.extern link_handler
x64_link_handler:
    SYSCALL_ENTER
    call link_handler
    SYSCALL_LEAVE

.extern transfer_handler
x64_transfer_handler:
    SYSCALL_ENTER
    call transfer_handler
    SYSCALL_LEAVE

invalid_syscall_handler:
    mov rax, 1 /* invalid system call error */
    sysretq

.macro SAVE_REGS
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rbp
    push rdi
    push rsi
    push rdx
    push rcx
    push rbx
    push rax
.endm

.macro RESTORE_REGS
    pop rax
    pop rbx
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rbp
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
.endm

/*
 *  Interrupt Handler
 */
.extern x64_handle_irq
.macro INTR_HANDLER num
.align 8
.globl interrupt_handler\num
interrupt_handler\num:
    cli

    push \num
    jmp do_handle_irq
.endm

do_handle_irq:
    /*
     *  The current RSP points to IST0, a CPU-local interrupt handler stack
     *  shared between threads.
     *
     *  Switch into the current thread's own kernel stack to save its thread
     *  context (IRET frame) since the interrupt could be from a timer device
     *  and the kernel may switches into another thread.
     *
     *    offset from RSI
     *
     *            +--------------------+
     *     48     |         SS         |
     *            +--------------------+
     *     40     |        RSP         |
     *            +--------------------+
     *     32     |       RFLAGS       |
     *            +--------------------+
     *     24     |         CS         |
     *            +--------------------+
     *     16     |        RIP         |
     *            +--------------------+
     *     8      |     IRQ Number     |
     *            +--------------------+
     *     0      |     saved RSI      |  <-- RSP/RSI points to here after `push rsi`.
     *            +--------------------+
     */
    push rsi
    mov rsi, rsp

    /* Check CS register in the IRET frame. */
    test qword ptr [rsi + 24], 3
    jnz 1f

    /* From kernel: extract RSP from the IRET frame. */
    mov rsp, [rsi + 40]
    jmp 2f
1:
    /* From user: use dedicated kernel stack. */
    mov rsp, gs:[ARCH_THREAD_RSP0] /* Use the kernel stack. */
2:
    /* Copy IRET frame. */
    push [rsi + 48]    /* SS */
    push [rsi + 40]    /* RSP */
    push [rsi + 32]    /* RFLAGS */
    push [rsi + 24]    /* CS */
    push [rsi + 16]    /* RIP */

    push rdi /* Save user RDI. */
    mov rdi, [rsi + 8] /* Get the IRQ number. */
    mov rsi, [rsi + 0] /* Restore RSI. */

    /* Save registers except RDI (we have already saved it above). */
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

    call x64_handle_irq

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

    test qword ptr [rsp + 16], 3
    jz 1f
1:
    iretq

.globl interrupt_handler_size
interrupt_handler_size:
.quad interrupt_handler33 - interrupt_handler32

halt:
    cli
    hlt
    jmp halt

/*
 *  Exception Handler
 */
.extern x64_handle_exception

.macro EXP_HANDLER num
.align 8
.globl exception_handler\num
exception_handler\num:
    cli

    /* Check CS register in the IRET frame to determine whether the exception
     * occur in the userspace. If so, do swapgs. */
    test qword ptr [rsp + 8], 3
    jz 1f /* From kernel: use IST0. */
    /* From user: use dedicated kernel stack. */
    push rsi
    mov rsi, rsp
    mov rsp, gs:[ARCH_THREAD_RSP0] /* Use the kernel stack. */
    push [rsi + 48]     /* SS */
    push [rsi + 40]     /* RSP */
    push [rsi + 32]     /* RFLAGS */
    push [rsi + 24]     /* CS */
    push [rsi + 16]     /* RIP */
    mov rsi, [rsi + 8]  /* pop RSI */
1:
    SAVE_REGS

    mov  rdi, \num
    call x64_handle_exception
    jmp ret_from_exp
.endm

.macro EXP_HANDLER_WITH_ERR num
.align 8
.globl  exception_handler\num
exception_handler\num:
    cli

    /* Check CS register in the IRET frame to determine whether the exception
     * occur in the userspace. If so, do swapgs. */
    test qword ptr [rsp + 16], 3
    jz 1f /* From kernel: use IST0. */

    /* From user: use dedicated kernel stack. The page fault handler may call
       an user pager and block. */
    push rsi
    mov rsi, rsp
    mov rsp, gs:[ARCH_THREAD_RSP0] /* Use the kernel stack. */

    push [rsi + 48]     /* SS */
    push [rsi + 40]     /* RSP */
    push [rsi + 32]     /* RFLAGS */
    push [rsi + 24]     /* CS */
    push [rsi + 16]     /* RIP */
    push [rsi + 8]      /* error code */
    mov rsi, [rsi + 0]  /* pop RSI */
1:
    SAVE_REGS

    mov rdi, \num
    mov rsi, [rsp + 8 * 15] /* error code */
    mov rdx, [rsp + 8 * 16] /* RIP */
    call x64_handle_exception
    jmp ret_from_exp
.endm


ret_from_exp:
    RESTORE_REGS
    add rsp, 8 /* skip error code */

    /* Check CS register in the IRET frame to determine whether the exception
     * occur in the userspace. If so, do swapgs. */
    test qword ptr [rsp + 8], 3
    jz 1f
1:
    iretq

EXP_HANDLER          0
EXP_HANDLER          1
EXP_HANDLER          2
EXP_HANDLER          3
EXP_HANDLER          4
EXP_HANDLER          5
EXP_HANDLER          6
EXP_HANDLER          7
EXP_HANDLER_WITH_ERR 8
EXP_HANDLER          9
EXP_HANDLER_WITH_ERR 10
EXP_HANDLER_WITH_ERR 11
EXP_HANDLER_WITH_ERR 12
EXP_HANDLER_WITH_ERR 13
EXP_HANDLER_WITH_ERR 14
EXP_HANDLER_WITH_ERR 15
EXP_HANDLER          16
EXP_HANDLER_WITH_ERR 17
EXP_HANDLER          18
EXP_HANDLER          19
EXP_HANDLER          20

INTR_HANDLER 32
INTR_HANDLER 33
INTR_HANDLER 34
INTR_HANDLER 35
INTR_HANDLER 36
INTR_HANDLER 37
INTR_HANDLER 38
INTR_HANDLER 39
INTR_HANDLER 40
INTR_HANDLER 41
INTR_HANDLER 42
INTR_HANDLER 43
INTR_HANDLER 44
INTR_HANDLER 45
INTR_HANDLER 46
INTR_HANDLER 47
INTR_HANDLER 48
INTR_HANDLER 49
INTR_HANDLER 50
INTR_HANDLER 51
INTR_HANDLER 52
INTR_HANDLER 53
INTR_HANDLER 54
INTR_HANDLER 55
INTR_HANDLER 56
INTR_HANDLER 57
INTR_HANDLER 58
INTR_HANDLER 59
INTR_HANDLER 60
INTR_HANDLER 61
INTR_HANDLER 62
INTR_HANDLER 63
INTR_HANDLER 64
INTR_HANDLER 65
INTR_HANDLER 66
INTR_HANDLER 67
INTR_HANDLER 68
INTR_HANDLER 69
INTR_HANDLER 70
INTR_HANDLER 71
INTR_HANDLER 72
INTR_HANDLER 73
INTR_HANDLER 74
INTR_HANDLER 75
INTR_HANDLER 76
INTR_HANDLER 77
INTR_HANDLER 78
INTR_HANDLER 79
INTR_HANDLER 80
INTR_HANDLER 81
INTR_HANDLER 82
INTR_HANDLER 83
INTR_HANDLER 84
INTR_HANDLER 85
INTR_HANDLER 86
INTR_HANDLER 87
INTR_HANDLER 88
INTR_HANDLER 89
INTR_HANDLER 90
INTR_HANDLER 91
INTR_HANDLER 92
INTR_HANDLER 93
INTR_HANDLER 94
INTR_HANDLER 95
INTR_HANDLER 96
INTR_HANDLER 97
INTR_HANDLER 98
INTR_HANDLER 99
INTR_HANDLER 100
INTR_HANDLER 101
INTR_HANDLER 102
INTR_HANDLER 103
INTR_HANDLER 104
INTR_HANDLER 105
INTR_HANDLER 106
INTR_HANDLER 107
INTR_HANDLER 108
INTR_HANDLER 109
INTR_HANDLER 110
INTR_HANDLER 111
INTR_HANDLER 112
INTR_HANDLER 113
INTR_HANDLER 114
INTR_HANDLER 115
INTR_HANDLER 116
INTR_HANDLER 117
INTR_HANDLER 118
INTR_HANDLER 119
INTR_HANDLER 120
INTR_HANDLER 121
INTR_HANDLER 122
INTR_HANDLER 123
INTR_HANDLER 124
INTR_HANDLER 125
INTR_HANDLER 126
INTR_HANDLER 127
INTR_HANDLER 128
INTR_HANDLER 129
INTR_HANDLER 130
INTR_HANDLER 131
INTR_HANDLER 132
INTR_HANDLER 133
INTR_HANDLER 134
INTR_HANDLER 135
INTR_HANDLER 136
INTR_HANDLER 137
INTR_HANDLER 138
INTR_HANDLER 139
INTR_HANDLER 140
INTR_HANDLER 141
INTR_HANDLER 142
INTR_HANDLER 143
INTR_HANDLER 144
INTR_HANDLER 145
INTR_HANDLER 146
INTR_HANDLER 147
INTR_HANDLER 148
INTR_HANDLER 149
INTR_HANDLER 150
INTR_HANDLER 151
INTR_HANDLER 152
INTR_HANDLER 153
INTR_HANDLER 154
INTR_HANDLER 155
INTR_HANDLER 156
INTR_HANDLER 157
INTR_HANDLER 158
INTR_HANDLER 159
INTR_HANDLER 160
INTR_HANDLER 161
INTR_HANDLER 162
INTR_HANDLER 163
INTR_HANDLER 164
INTR_HANDLER 165
INTR_HANDLER 166
INTR_HANDLER 167
INTR_HANDLER 168
INTR_HANDLER 169
INTR_HANDLER 170
INTR_HANDLER 171
INTR_HANDLER 172
INTR_HANDLER 173
INTR_HANDLER 174
INTR_HANDLER 175
INTR_HANDLER 176
INTR_HANDLER 177
INTR_HANDLER 178
INTR_HANDLER 179
INTR_HANDLER 180
INTR_HANDLER 181
INTR_HANDLER 182
INTR_HANDLER 183
INTR_HANDLER 184
INTR_HANDLER 185
INTR_HANDLER 186
INTR_HANDLER 187
INTR_HANDLER 188
INTR_HANDLER 189
INTR_HANDLER 190
INTR_HANDLER 191
INTR_HANDLER 192
INTR_HANDLER 193
INTR_HANDLER 194
INTR_HANDLER 195
INTR_HANDLER 196
INTR_HANDLER 197
INTR_HANDLER 198
INTR_HANDLER 199
INTR_HANDLER 200
INTR_HANDLER 201
INTR_HANDLER 202
INTR_HANDLER 203
INTR_HANDLER 204
INTR_HANDLER 205
INTR_HANDLER 206
INTR_HANDLER 207
INTR_HANDLER 208
INTR_HANDLER 209
INTR_HANDLER 210
INTR_HANDLER 211
INTR_HANDLER 212
INTR_HANDLER 213
INTR_HANDLER 214
INTR_HANDLER 215
INTR_HANDLER 216
INTR_HANDLER 217
INTR_HANDLER 218
INTR_HANDLER 219
INTR_HANDLER 220
INTR_HANDLER 221
INTR_HANDLER 222
INTR_HANDLER 223
INTR_HANDLER 224
INTR_HANDLER 225
INTR_HANDLER 226
INTR_HANDLER 227
INTR_HANDLER 228
INTR_HANDLER 229
INTR_HANDLER 230
INTR_HANDLER 231
INTR_HANDLER 232
INTR_HANDLER 233
INTR_HANDLER 234
INTR_HANDLER 235
INTR_HANDLER 236
INTR_HANDLER 237
INTR_HANDLER 238
INTR_HANDLER 239
INTR_HANDLER 240
INTR_HANDLER 241
INTR_HANDLER 242
INTR_HANDLER 243
INTR_HANDLER 244
INTR_HANDLER 245
INTR_HANDLER 246
INTR_HANDLER 247
INTR_HANDLER 248
INTR_HANDLER 249
INTR_HANDLER 250
INTR_HANDLER 251
INTR_HANDLER 252
INTR_HANDLER 253
INTR_HANDLER 254
INTR_HANDLER 255
