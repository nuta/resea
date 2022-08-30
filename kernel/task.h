#pragma once
#include <types.h>
#include <list.h>
#include <arch_types.h>

#define TASK_PRIORITY_MAX 8
#define TASK_NAME_LEN 16

#define TASK_RUNNABLE 1
#define TASK_BLOCKED  2

struct task {
    /// The arch-specific fields.
    struct arch_task arch;
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
