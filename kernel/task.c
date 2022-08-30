#include "task.h"
#include "arch.h"
#include <list.h>

static list_t runqueues[TASK_PRIORITY_MAX];

static void enqueue_task(struct task *task) {
    list_push_back(&runqueues[task->priority], &task->next);
}

static struct task *scheduler(struct task *current) {
    if (current != IDLE_TASK && current->state == TASK_RUNNABLE) {
        // The current task is still runnable. Enqueue into the runqueue.
        enqueue_task(current);
    }

    // Look for the task with the highest priority. Tasks with the same priority
    // is scheduled in round-robin fashion.
    for (int i = 0; i < TASK_PRIORITY_MAX; i++) {
        struct task *next = LIST_POP_FRONT(&runqueues[i], struct task, next);
        if (next) {
            return next;
        }
    }

    return IDLE_TASK;
}

void task_switch(void) {
    struct task *prev = CURRENT_TASK;
    struct task *next = scheduler(prev);
    DEBUG_ASSERT(next != prev);

    CURRENT_TASK = next;
    arch_task_switch(prev, next);
}

/// Initializes the task subsystem.
void task_init(void) {
    for (int i = 0; i < TASK_PRIORITY_MAX; i++) {
        list_init(&runqueues[i]);
    }
}

