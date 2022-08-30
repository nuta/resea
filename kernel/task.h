#pragma once
#include <arch_types.h>
#include <list.h>
#include <types.h>

#define NUM_TASKS_MAX     32
#define TASK_PRIORITY_MAX 8
#define TASK_NAME_LEN     16

#define TASK_UNUSED   0
#define TASK_RUNNABLE 1
#define TASK_BLOCKED  2

#define IDLE_TASK    (arch_cpuvar_get()->idle_task)
#define CURRENT_TASK (arch_cpuvar_get()->current_task)

struct task {
    /// The arch-specific fields.
    struct arch_task arch;
    struct arch_vm vm;
    task_t tid;

    int state;
    int priority;
    int quantum;

    // struct message m;
    // notifications_t notifications;
    list_t senders;
    list_elem_t next;
    // caps

    task_t waiting_for;

    struct task *pager;
    uint16_t ref_count;

    char name[TASK_NAME_LEN];
};

void task_init(void);
