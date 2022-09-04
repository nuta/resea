#pragma once
#include "arch.h"
#include <list.h>
#include <message.h>
#include <types.h>

#define TASK_PRIORITY_MAX 8

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

    struct task *pager;
    uint16_t ref_count;

    task_t waiting_for;
    notifications_t notifications;
    struct message m;

    char name[TASK_NAME_LEN];
};

struct task *get_task_by_tid(task_t tid);
task_t task_create(const char *name, uaddr_t ip, struct task *pager,
                   unsigned flags);
error_t task_destroy(struct task *task);
void task_resume(struct task *task);
void task_block(struct task *task);
void task_switch(void);
void task_init(void);
