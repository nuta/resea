#ifndef __TASK_H__
#define __TASK_H__

#include <arch.h>
#include <message.h>
#include <types.h>
#include "memory.h"

#define TASK_TIME_SLICE ((10 * TICK_HZ) / 1000) /* 10 milliseconds */
#define TASKS_MAX       16
#define TASK_NAME_LEN   16

//
// Task states.
//

/// The task struct is not being used.
#define TASK_UNUSED 0
/// The task is being created.
#define TASK_CREATED 1
/// The task is running or is queued in the runqueue.
#define TASK_RUNNABLE 2
/// The task is waiting for a receiver task in IPC.
#define TASK_SENDING 3
/// The task is waiting for a sender task in IPC.
#define TASK_RECEIVING 4
/// The task has exited. Waiting for the pager to destructs it.
#define TASK_EXITED 5

/// Determines if the current task has the given capability.
#define CAPABLE(cap) ((CURRENT->caps & (cap)) != 0)

// struct arch_cpuvar *
#define ARCH_CPUVAR (&get_cpuvar()->arch)

/// The current task of the current CPU (`struct task *`).
#define CURRENT (get_cpuvar()->current_task)
/// The idle task of the current CPU (`struct task *`).
#define IDLE_TASK (&get_cpuvar()->idle_task)

/// The task struct (so-called Task Control Block).
struct task {
    /// The arch-specific fields.
    struct arch_task arch;
    /// The task ID. Starts with 1.
    tid_t tid;
    /// The state.
    int state;
    /// The name of task terminated by NUL.
    char name[TASK_NAME_LEN];
    /// Capabilities (allowed operations).
    caps_t caps;
    /// The page table.
    struct vm vm;
    /// The pager task. When a page fault or an exception (e.g. divide by zero)
    /// occurred, the kernel sends a message to the pager to allow it to
    /// resolve the faults (or kill the task).
    struct task *pager;
    /// The remaining time slice in ticks. If this value reaches 0, the kernel
    /// switches into the next task (so-called preemptive context switching).
    unsigned quantum;
    /// The message buffer.
    struct message *buffer;
    /// The acceptable sender task ID. If it's IPC_ANY, the task accepts
    /// messages from any tasks.
    tid_t src;
    /// The pending notifications. It's cleared when the task received them as
    /// an message (NOTIFICATIONS_MSG).
    notifications_t notifications;
    /// The IPC timeout in milliseconds. When it become 0, the kernel notify the
    /// task with `NOTIFY_TIMER`.
    msec_t timeout;
    /// The queue of tasks that are waiting for this task to get ready for
    /// receiving a message. If this task gets ready, it resumes all threads in
    /// this queue.
    list_t senders;
    /// The table tasks that are waiting for this task to get ready for
    /// receiving a message. When this task become TASK_RECEIVING, it notifies
    /// all threads registered in this table with `NOTIFY_READY`.
    ///
    /// Note that task ID is 1-origin, i.e., `listened_by[1]` is used by task
    /// #2, not #1.
    ///
    /// FIXME: This requires O(n) operaions.
    bool listened_by[TASKS_MAX];
    /// A (intrusive) list element in the runqueue.
    list_elem_t runqueue_next;
    /// A (intrusive) list element in a sender queue.
    list_elem_t sender_next;
};

/// CPU-local variables.
struct cpuvar {
    struct arch_cpuvar arch;
    struct task *current_task;
    struct task idle_task;
};

error_t task_create(struct task *task, const char *name, vaddr_t ip,
                    struct task *pager, caps_t caps);
error_t task_destroy(struct task *task);
NORETURN void task_exit(enum exception_type exp);
void task_set_state(struct task *task, int state);
void task_notify(struct task *task, notifications_t notifications);
struct task *task_lookup(tid_t tid);
void task_switch(void);
error_t task_listen_irq(struct task *task, unsigned irq);
error_t task_unlisten_irq(struct task *task, unsigned irq);
void handle_irq(unsigned irq);
void task_dump(void);
void task_init(void);

// Implemented in arch.
void lock(void);
void unlock(void);
int mp_cpuid(void);
int mp_num_cpus(void);
void mp_reschedule(void);
error_t arch_task_create(struct task *task, vaddr_t ip);
void arch_task_destroy(struct task *task);
void arch_task_switch(struct task *prev, struct task *next);
void arch_enable_irq(unsigned irq);
void arch_disable_irq(unsigned irq);

#endif
