#include "main.h"
#include "kdebug.h"
#include "memory.h"
#include "printk.h"
#include "syscall.h"
#include "task.h"

/// Initializes the kernel and starts the first task.
void kmain(void) {
    printf("\nBooting Resea...\n");
    memory_init();
    task_init();
    mp_start();

    // Create the first userland task (init).
    struct task *task = task_lookup(INIT_TASK_TID);
    ASSERT(task);
    task_create(task, "init", INITFS_ADDR, 0, CAP_ALL);

    mpmain();
}

void mpmain(void) {
    stack_set_canary();

    // Initialize the idle task for this CPU.
    IDLE_TASK->tid = 0;
    task_create(IDLE_TASK, "(idle)", 0, 0, CAP_IPC);
    CURRENT = IDLE_TASK;

    // Do the very first context switch on this CPU.
    INFO("Booted CPU #%d", mp_cpuid());
    task_switch();

    // We're now in the current CPU's idle task.
    while (true) {
        // Halt the CPU until an interrupt arrives...
        arch_idle();
        // Handled an interrupt. Try switching into a task resumed by an
        // interrupt message.
        task_switch();
    }
}
