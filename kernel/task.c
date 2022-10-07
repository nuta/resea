#include "task.h"
#include "arch.h"
#include "ipc.h"
#include "memory.h"
#include "printk.h"
#include <list.h>
#include <string.h>

static struct task tasks[NUM_TASKS_MAX];
static struct task idle_tasks[NUM_CPUS_MAX];
static list_t runqueues[TASK_PRIORITY_MAX];

static void enqueue_task(struct task *task) {
    list_push_back(&runqueues[task->priority], &task->next);
}

static struct task *scheduler(struct task *current) {
    // Look for the task with the highest priority. Tasks with the same priority
    // is scheduled in round-robin fashion.
    for (int i = 0; i < TASK_PRIORITY_MAX; i++) {
        struct task *next = LIST_POP_FRONT(&runqueues[i], struct task, next);
        if (next) {
            return next;
        }
    }

    if (current->state == TASK_RUNNABLE) {
        // No other task is runnable, so we can continue running the current
        // task.
        return current;
    }

    // No task is runnable.
    return IDLE_TASK;
}

static task_t init_task_struct(const char *name, uaddr_t ip, struct task *pager,
                               unsigned flags, task_t tid, struct task *task) {
    task->tid = tid;
    task->state = TASK_BLOCKED;
    task->priority = 0;
    task->quantum = 0;
    task->waiting_for = 0;
    task->pager = pager;
    task->ref_count = 1;

    strncpy2(task->name, name, sizeof(task->name));
    list_elem_init(&task->next);
    list_init(&task->senders);
    list_init(&task->pages);

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

void task_switch(void) {
    struct task *prev = CURRENT_TASK;
    struct task *next = scheduler(prev);

    if (next == prev) {
        // Keep running the current task.
        return;
    }

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

struct task *get_task_by_tid_unchecked(task_t tid) {
    if (0 < tid && tid < NUM_TASKS_MAX) {
        return &tasks[tid - 1];
    }

    return NULL;
}

struct task *get_task_by_tid(task_t tid) {
    struct task *task = get_task_by_tid_unchecked(tid);
    if (!task || task->state == TASK_UNUSED) {
        return NULL;
    }

    return task;
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

    struct task *task = get_task_by_tid_unchecked(tid);
    DEBUG_ASSERT(task != NULL);
    return init_task_struct(name, ip, pager, flags, tid, task);
}

error_t task_destroy(struct task *task) {
    DEBUG_ASSERT(task != CURRENT_TASK);
    DEBUG_ASSERT(task != IDLE_TASK);
    DEBUG_ASSERT(task->state != TASK_UNUSED);
    DEBUG_ASSERT(task->ref_count > 0);

    if (task->tid == 1) {
        WARN_DBG("tried to destroy the task #1");
        return ERR_INVALID_ARG;
    }

    task->pager->ref_count--;
    task->ref_count--;

    if (task->ref_count > 0) {
        WARN_DBG("%s (#%d) is still referenced from %d tasks", task->name,
                 task->tid, task->ref_count);
        return ERR_STILL_USED;
    }

    // Abort sender IPC operations.
    LIST_FOR_EACH (sender, &task->senders, struct task, next) {
        // notify(sender, NOTIFY_ABORTED);
    }

    list_remove(&task->next);

    arch_vm_destroy(&task->vm);
    arch_task_destroy(task);
    pm_free_list(&task->pages);
    task->state = TASK_UNUSED;
    return OK;
}

__noreturn void task_exit(int exception) {
    struct task *pager = CURRENT_TASK->pager;
    ASSERT(pager != NULL);

    struct message m;
    m.type = EXCEPTION_MSG;
    m.exception.task = CURRENT_TASK->tid;
    m.exception.reason = exception;
    error_t err = ipc(CURRENT_TASK->pager, IPC_DENY,
                      (__user struct message *) &m, IPC_SEND | IPC_KERNEL);

    if (err != OK) {
        WARN("%s: failed to send an exit message to '%s': %s",
             CURRENT_TASK->name, pager->name, err2str(err));
    }

    task_block(CURRENT_TASK);
    task_switch();

    UNREACHABLE();
}

void task_init(void) {
    for (int i = 0; i < TASK_PRIORITY_MAX; i++) {
        list_init(&runqueues[i]);
    }
}

void task_mp_init(void) {
    init_task_struct("(idle)", 0, NULL, 0, 0, &idle_tasks[CPUVAR->id]);
    IDLE_TASK = &idle_tasks[CPUVAR->id];
    CURRENT_TASK = IDLE_TASK;
}
