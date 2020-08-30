#ifndef __TASK_H__
#define __TASK_H__

#include <config.h>
#include <types.h>
#include <arch.h>
#include <message.h>
#include <list.h>
#include <bitmap.h>

/// The context switching time slice (# of ticks).
#define TASK_TIME_SLICE ((CONFIG_TASK_TIME_SLICE_MS * TICK_HZ) / 1000)
STATIC_ASSERT(TASK_TIME_SLICE > 0);

//
// Task states.
//

/// The task struct is not being used.
#define TASK_UNUSED 0
/// The task is running or is queued in the runqueue.
#define TASK_RUNNABLE 1
/// The task is waiting for a receiver/sender task in IPC.
#define TASK_BLOCKED 2

// struct arch_cpuvar *
#define ARCH_CPUVAR (&get_cpuvar()->arch)

/// The current task of the current CPU (`struct task *`).
#define CURRENT (get_cpuvar()->current_task)
/// The idle task of the current CPU (`struct task *`).
#define IDLE_TASK (&get_cpuvar()->idle_task)

// Task capabilities.
#define CAP_TASK    0
#define CAP_IRQ     1
#define CAP_IO      2
#define CAP_MAP     3
#define CAP_KDEBUG  4
#define CAP_MAX     4

#define CAPABLE(task, cap) (bitmap_get((task)->caps, sizeof((task)->caps), cap) != 0)

/// The task struct (so-called Task Control Block).
struct task {
    /// The arch-specific fields.
    struct arch_task arch;
    /// The task ID. Starts with 1.
    task_t tid;
    /// The state.
    int state;
    /// The name of task terminated by NUL.
    char name[CONFIG_TASK_NAME_LEN];
    /// Flags.
    unsigned flags;
    /// Number of references to this task.
    unsigned ref_count;
    /// The pager task. When a page fault or an exception (e.g. divide by zero)
    /// occurred, the kernel sends a message to the pager to allow it to
    /// resolve the faults (or kill the task).
    struct task *pager;
    /// The remaining time slice in ticks. If this value reaches 0, the kernel
    /// switches into the next task (so-called preemptive context switching).
    unsigned quantum;
    /// The message buffer.
    struct message m;
    /// The acceptable sender task ID. If it's IPC_ANY, the task accepts
    /// messages from any tasks.
    task_t src;
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
    /// A (intrusive) list element in the runqueue.
    list_elem_t runqueue_next;
    /// A (intrusive) list element in a sender queue.
    list_elem_t sender_next;
    /// Capabilities (bitmap).
    uint8_t caps[BITMAP_SIZE(CAP_MAX)];
};

/// CPU-local variables.
struct cpuvar {
    struct arch_cpuvar arch;
    struct task *current_task;
    struct task idle_task;
};

__mustuse error_t task_create(struct task *task, const char *name, vaddr_t ip,
                            struct task *pager, unsigned flags);
__mustuse error_t task_destroy(struct task *task);
__noreturn void task_exit(enum exception_type exp);
void task_block(struct task *task);
void task_resume(struct task *task);
struct task *task_lookup(task_t tid);
struct task *task_lookup_unchecked(task_t tid);
void task_switch(void);
__mustuse error_t task_map_page(struct task *task, vaddr_t vaddr, paddr_t paddr,
                                paddr_t kpage, unsigned flags);
__mustuse error_t task_unmap_page(struct task *task, vaddr_t vaddr);
__mustuse error_t task_listen_irq(struct task *task, unsigned irq);
__mustuse error_t task_unlisten_irq(unsigned irq);
void handle_timer_irq(void);
void handle_irq(unsigned irq);
void handle_page_fault(vaddr_t addr, vaddr_t ip, unsigned fault);
void task_dump(void);
void task_init(void);

// Implemented in arch.
void lock(void);
void panic_lock(void);
void unlock(void);
void mp_start(void);
int mp_self(void);
int mp_num_cpus(void);
void mp_reschedule(void);
__mustuse error_t arch_task_create(struct task *task, vaddr_t ip);
void arch_task_destroy(struct task *task);
void arch_task_switch(struct task *prev, struct task *next);
void arch_enable_irq(unsigned irq);
void arch_disable_irq(unsigned irq);
__mustuse error_t arch_map_page(struct task *task, vaddr_t vaddr, paddr_t paddr,
                          paddr_t kpage, unsigned flags);
__mustuse error_t arch_unmap_page(struct task *task, vaddr_t vaddr);
paddr_t vm_resolve(struct task *task, vaddr_t vaddr);

#endif
