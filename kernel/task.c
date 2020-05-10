#include "task.h"
#include <arch.h>
#include <list.h>
#include <cstring.h>
#include "ipc.h"
#include "kdebug.h"
#include "memory.h"
#include "message.h"
#include "printk.h"
#include "syscall.h"

/// All tasks.
static struct task tasks[TASKS_MAX];
/// A queue of runnable tasks excluding currently running tasks.
static list_t runqueue;
/// IRQ owners.
static struct task *irq_owners[IRQ_MAX];

/// Returns the task struct for the task ID. It returns NULL if the ID is
/// invalid.
struct task *task_lookup(task_t tid) {
    if (tid <= 0 || tid > TASKS_MAX) {
        return NULL;
    }

    return &tasks[tid - 1];
}

/// Initializes a task struct.
error_t task_create(struct task *task, const char *name, vaddr_t ip,
                    struct task *pager, caps_t caps) {
    if (task->state != TASK_UNUSED) {
        return ERR_ALREADY_EXISTS;
    }

    // Initialize the page table.
    error_t err;
    if ((err = vm_create(&task->vm)) != OK) {
        return err;
    }

    // Do arch-specific initialization.
    if ((err = arch_task_create(task, ip)) != OK) {
        vm_destroy(&task->vm);
        return err;
    }

    // Initialize fields.
    TRACE("new task #%d: %s", task->tid, name);
    task->state = TASK_CREATED;
    task->caps = caps;
    task->notifications = 0;
    task->pager = pager;
    task->bulk_ptr = 0;
    task->timeout = 0;
    task->quantum = 0;
    strncpy(task->name, name, sizeof(task->name));
    list_init(&task->senders);
    list_nullify(&task->runqueue_next);
    list_nullify(&task->sender_next);

    // Append the newly created task into the runqueue.
    if (task != IDLE_TASK) {
        task_set_state(task, TASK_RUNNABLE);
    }

    return OK;
}

/// Frees the task data structures and make it unused.
error_t task_destroy(struct task *task) {
    ASSERT(task != CURRENT);
    ASSERT(task != IDLE_TASK);

    if (task->tid == INIT_TASK_TID) {
        TRACE("%s: tried to destroy the init task", task->name);
        return ERR_INVALID_ARG;
    }

    if (task->state == TASK_UNUSED) {
        return ERR_INVALID_ARG;
    }

    TRACE("destroying %s...", task->name);
    list_remove(&task->runqueue_next);
    list_remove(&task->sender_next);
    vm_destroy(&task->vm);
    arch_task_destroy(task);
    task->state = TASK_UNUSED;

    // Abort sender IPC operations.
    LIST_FOR_EACH (sender, &task->senders, struct task, sender_next) {
        notify(sender, NOTIFY_ABORTED);
        list_remove(&sender->sender_next);
    }

    for (task_t tid = 1; tid <= TASKS_MAX; tid++) {
        struct task *task2 = task_lookup(tid);
        DEBUG_ASSERT(task2);

        // Ensure that this task is not a pager task.
        if (task2->state != TASK_UNUSED && task2->pager == task) {
            PANIC("a pager task '%s' (#%d) is being killed", task->name,
                  task->tid);
        }
    }

    for (unsigned irq = 0; irq < IRQ_MAX; irq++) {
        if (irq_owners[irq] == task) {
            arch_disable_irq(irq);
            irq_owners[irq] = NULL;
        }
    }

    return OK;
}

/// Exits the current task. `exp` is the reason why the task is being exited.
NORETURN void task_exit(enum exception_type exp) {
    ASSERT(CURRENT != IDLE_TASK);

    // Tell its pager that this task has exited.
    struct message m;
    m.type = EXCEPTION_MSG;
    m.exception.task = CURRENT->tid;
    m.exception.exception = exp;
    ipc(CURRENT->pager, 0, &m, IPC_SEND | IPC_KERNEL);

    // Wait until the pager task destroys this task...
    CURRENT->state = TASK_EXITED;
    task_switch();
    UNREACHABLE();
}

/// Updates a task's state.
void task_set_state(struct task *task, int state) {
    DEBUG_ASSERT(task->state != state);

    task->state = state;
    if (state == TASK_RUNNABLE) {
        list_push_back(&runqueue, &task->runqueue_next);
        mp_reschedule();
    }
}

/// Picks the next task to run.
static struct task *scheduler(struct task *current) {
    if (current != IDLE_TASK && current->state == TASK_RUNNABLE) {
        // The current task is still runnable. Enqueue into the runqueue.
        list_push_back(&runqueue, &current->runqueue_next);
    }

    struct task *next = LIST_POP_FRONT(&runqueue, struct task, runqueue_next);
    return (next) ? next : IDLE_TASK;
}

/// Do a context switch: save the current register state on the stack and
/// restore the next thread's state.
void task_switch(void) {
    stack_check();

    struct task *prev = CURRENT;
    struct task *next = scheduler(prev);
    next->quantum = TASK_TIME_SLICE;
    if (next == prev) {
        // No runnable threads other than the current one. Continue executing
        // the current thread.
        return;
    }

    CURRENT = next;
    arch_task_switch(prev, next);

    stack_check();
}

error_t task_listen_irq(struct task *task, unsigned irq) {
    if (irq >= IRQ_MAX) {
        return ERR_INVALID_ARG;
    }

    if (irq_owners[irq]) {
        return ERR_ALREADY_EXISTS;
    }

    irq_owners[irq] = task;
    arch_enable_irq(irq);
    TRACE("enabled IRQ: task=%s, vector=%d", task->name, irq);
    return OK;
}

error_t task_unlisten_irq(struct task *task, unsigned irq) {
    if (irq >= IRQ_MAX) {
        return ERR_INVALID_ARG;
    }

    if (irq_owners[irq] != task) {
        return ERR_NOT_PERMITTED;
    }

    arch_disable_irq(irq);
    irq_owners[irq] = NULL;
    TRACE("disabled IRQ: task=%s, vector=%d", task->name, irq);
    return OK;
}

void handle_irq(unsigned irq) {
    if (irq == TIMER_IRQ) {
        // Handles timer interrupts. The timer fires this IRQ every 1/TICK_HZ
        // seconds.

        // Handle task timeouts.
        if (mp_is_bsp()) {
            for (int i = 0; i < TASKS_MAX; i++) {
                struct task *task = &tasks[i];
                if (task->state == TASK_UNUSED || !task->timeout) {
                    continue;
                }

                task->timeout--;
                if (!task->timeout) {
                    notify(task, NOTIFY_TIMER);
                }
            }
        }

        // Switch task if the current task has spend its time slice.
        DEBUG_ASSERT(CURRENT->quantum > 0);
        CURRENT->quantum--;
        if (!CURRENT->quantum) {
            task_switch();
        }
    } else {
        struct task *owner = irq_owners[irq];
        if (owner) {
            notify(owner, NOTIFY_IRQ);
        }
    }
}

void task_dump(void) {
    const char *states[] = {
        [TASK_UNUSED] = "unused",        [TASK_CREATED] = "created",
        [TASK_EXITED] = "exited",        [TASK_RUNNABLE] = "runnable",
        [TASK_RECEIVING] = "receiveing", [TASK_SENDING] = "sending",
    };

    for (unsigned i = 0; i < TASKS_MAX; i++) {
        struct task *task = &tasks[i];
        if (task->state == TASK_UNUSED) {
            continue;
        }

        DPRINTK("#%d %s: state=%s, src=%d\n", task->tid, task->name,
                states[task->state], task->src);
        if (!list_is_empty(&task->senders)) {
            DPRINTK("  senders:\n");
            LIST_FOR_EACH (sender, &task->senders, struct task, sender_next) {
                DPRINTK("    - #%d %s\n", sender->tid, sender->name);
            }
        }
    }
}

/// Initializes the task subsystem.
void task_init(void) {
    list_init(&runqueue);
    for (int i = 0; i < TASKS_MAX; i++) {
        tasks[i].state = TASK_UNUSED;
        tasks[i].tid = i + 1;
    }

    for (int i = 0; i < IRQ_MAX; i++) {
        irq_owners[i] = NULL;
    }
}
