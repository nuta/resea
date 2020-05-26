.text

// void arm_task_switch(vaddr_t *prev_sp, vaddr_t next_sp);
.global arm_task_switch
.thumb_func
arm_task_switch:
    push {lr}

    // Save r4-r11.
    push {r4-r7}
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    push {r4-r7}

    // Save prev's sp.
    mov r2, sp
    str r2, [r0]

    // Restore next's sp.
    mov sp, r1

    // Restore r4-r11.
    pop {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    pop {r4-r7}

    pop {r0}
    bx r0

.global arm_start_task
.thumb_func
arm_start_task:
    bl stack_set_canary
    pop {r0}
    movs r1, #1
    orrs r0, r1
    bx r0
