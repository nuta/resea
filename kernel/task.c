#include "task.h"
#include "arch.h"
#include "memory.h"
#include "printk.h"
#include <list.h>

static struct task tasks[NUM_TASKS_MAX];
static list_t runqueues[TASK_PRIORITY_MAX];

static void enqueue_task(struct task *task) {
    list_push_back(&runqueues[task->priority], &task->next);
}

static struct task *scheduler(struct task *current) {
    if (current != IDLE_TASK && current->state == TASK_RUNNABLE) {
        // The current task is still runnable. Enqueue into the runqueue.
        enqueue_task(current);
        // FIXME:
        return IDLE_TASK;
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

    if (next == prev) {
        // Keep running the current task.
        return;
    }

    TRACE("switch to task %d", next->tid);
    CURRENT_TASK = next;
    arch_task_switch(prev, next);
}

static task_t get_unused_tid(void) {
    for (task_t i = 0; i < NUM_TASKS_MAX; i++) {
        if (tasks[i].state == TASK_UNUSED) {
            return i + 1;
        }
    }

    return 0;
}

struct task *get_task_by_tid(task_t tid) {
    if (0 < tid && tid < NUM_TASKS_MAX) {
        return &tasks[tid - 1];
    }

    return NULL;
}

void task_block(struct task *task) {
    DEBUG_ASSERT(task->state == TASK_RUNNABLE);
    task->state = TASK_BLOCKED;
}

void task_resume(struct task *task) {
    DEBUG_ASSERT(task->state == TASK_BLOCKED);
    task->state = TASK_RUNNABLE;
    enqueue_task(task);
}

task_t task_create(const char *name, uaddr_t ip, struct task *pager,
                   unsigned flags) {
    task_t tid = get_unused_tid();
    if (!tid) {
        return ERR_TOO_MANY_TASKS;
    }

    struct task *task = get_task_by_tid(tid);
    DEBUG_ASSERT(task != NULL);
    task->tid = tid;
    task->state = TASK_BLOCKED;
    task->priority = 0;
    task->quantum = 0;
    task->waiting_for = 0;
    task->pager = pager;
    task->ref_count = 1;

    list_init(&task->senders);

    error_t err = arch_vm_init(&task->vm);
    if (err != OK) {
        return err;
    }

    err = arch_task_init(task, ip);
    if (err != OK) {
        // TODO: free task->vm
        return err;
    }

    return tid;
}

error_t task_destroy(struct task *task) {
    NYI();
    return OK;
}

void task_init(void) {
    for (int i = 0; i < TASK_PRIORITY_MAX; i++) {
        list_init(&runqueues[i]);
    }
}
