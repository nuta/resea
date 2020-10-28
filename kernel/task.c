#include <arch.h>
#include <list.h>
#include <config.h>
#include <string.h>
#include <message.h>
#include "task.h"
#include "ipc.h"
#include "kdebug.h"
#include "printk.h"
#include "syscall.h"

/// All tasks.
static struct task tasks[CONFIG_NUM_TASKS];
/// A queue of runnable tasks excluding currently running tasks. Lower index
/// means higher priority.
static list_t runqueues[TASK_PRIORITY_MAX];
/// IRQ owners.
static struct task *irq_owners[IRQ_MAX];

static void enqueue_task(struct task *task) {
    list_push_back(&runqueues[task->priority], &task->runqueue_next);
}

/// Returns the task struct for the task ID. It returns NULL if the ID is
/// invalid.
struct task *task_lookup_unchecked(task_t tid) {
    if (tid <= 0 || tid > CONFIG_NUM_TASKS) {
        return NULL;
    }

    return &tasks[tid - 1];
}

/// Returns the task struct for the task ID. It returns NULL if the ID is
/// invalid or the task is not in use (TASK_UNUSED).
struct task *task_lookup(task_t tid) {
    struct task *task = task_lookup_unchecked(tid);
    if (!task || task->state == TASK_UNUSED) {
        return NULL;
    }

    return task;
}

/// Initializes a task struct.
error_t task_create(struct task *task, const char *name, vaddr_t ip,
                    struct task *pager, unsigned flags) {
    if (task->state != TASK_UNUSED) {
        return ERR_ALREADY_EXISTS;
    }

    unsigned allowed_flags = TASK_ALL_CAPS | TASK_ABI_EMU;
    if ((flags & ~allowed_flags) != 0) {
        WARN_DBG("unknown task flags (%x)", flags);
        return ERR_INVALID_ARG;
    }

#ifndef CONFIG_ABI_EMU
    if ((flags & TASK_ABI_EMU) != 0) {
        WARN_DBG("ABI emulation is not enabled");
        return ERR_UNAVAILABLE;
    }
#endif

    // Do arch-specific initialization.
    error_t err;
    if ((err = arch_task_create(task, ip)) != OK) {
        return err;
    }

    // Initialize fields.
    TRACE("new task #%d: %s (pager=%s)",
          task->tid, name, pager ? pager->name : NULL);
    task->state = TASK_BLOCKED;
    task->flags = flags;
    task->notifications = 0;
    task->pager = pager;
    task->src = IPC_DENY;
    task->timeout = 0;
    task->quantum = 0;
    task->priority = TASK_PRIORITY_MAX - 1;
    task->ref_count = 0;
    bitmap_fill(task->caps, sizeof(task->caps), (flags & TASK_ALL_CAPS) != 0);
    strncpy(task->name, name, sizeof(task->name));
    list_init(&task->senders);
    list_nullify(&task->runqueue_next);
    list_nullify(&task->sender_next);

    if (pager) {
        pager->ref_count++;
    }

    // Append the newly created task into the runqueue.
    if (task != IDLE_TASK && ((flags & TASK_SCHED) == 0)) {
        task_resume(task);
    }

    return OK;
}

/// Frees the task data structures and make it unused.
error_t task_destroy(struct task *task) {
    ASSERT(task != CURRENT);
    ASSERT(task != IDLE_TASK);

    if (task->tid == INIT_TASK) {
        WARN_DBG("tried to destroy the init task");
        return ERR_INVALID_ARG;
    }

    if (task->state == TASK_UNUSED) {
        return ERR_INVALID_ARG;
    }

    if (task->ref_count > 0) {
        WARN_DBG("%s (#%d) is still referenced from %d tasks",
                 task->name, task->tid, task->ref_count);
        return ERR_IN_USE;
    }

    TRACE("destroying %s...", task->name);
    list_remove(&task->runqueue_next);
    list_remove(&task->sender_next);
    arch_task_destroy(task);
    task->state = TASK_UNUSED;

    if (task->pager) {
        task->pager->ref_count--;
    }

    // Abort sender IPC operations.
    LIST_FOR_EACH (sender, &task->senders, struct task, sender_next) {
        notify(sender, NOTIFY_ABORTED);
        list_remove(&sender->sender_next);
    }

    // Release IRQ ownership.
    for (unsigned irq = 0; irq < IRQ_MAX; irq++) {
        if (irq_owners[irq] == task) {
            arch_disable_irq(irq);
            irq_owners[irq] = NULL;
        }
    }

    return OK;
}

/// Exits the current task. `exp` is the reason why the task is being exited.
__noreturn void task_exit(enum exception_type exp) {
    ASSERT(CURRENT != IDLE_TASK);

    if (!CURRENT->pager) {
        PANIC("the initial task tried to exit");
    }

    // Tell its pager that this task has exited.
    struct message m;
    m.type = EXCEPTION_MSG;
    m.exception.task = CURRENT->tid;
    m.exception.exception = exp;
    error_t err = ipc(CURRENT->pager, 0, (__user struct message *) &m,
                      IPC_SEND | IPC_KERNEL);
    OOPS_OK(err);

    // Wait until the pager task destroys this task...
    CURRENT->state = TASK_BLOCKED;
    CURRENT->src = IPC_DENY;
    task_switch();
    UNREACHABLE();
}

/// Suspends a task. Don't forget to update `task->src` as well!
void task_block(struct task *task) {
    DEBUG_ASSERT(task->state == TASK_RUNNABLE);
    task->state = TASK_BLOCKED;
}

/// Resumes a task.
void task_resume(struct task *task) {
    DEBUG_ASSERT(task->state == TASK_BLOCKED);
    task->state = TASK_RUNNABLE;
    enqueue_task(task);
    mp_reschedule();
}

/// Updates the scheduling policy for the task.
error_t task_schedule(struct task *task, int priority) {
    if (priority >= TASK_PRIORITY_MAX) {
        return ERR_INVALID_ARG;
    }

    task->priority = priority;
    if (task->state == TASK_RUNNABLE) {
        list_remove(&task->runqueue_next);
        enqueue_task(task);
    }

    return OK;
}

/// Picks the next task to run.
static struct task *scheduler(struct task *current) {
    if (current != IDLE_TASK && current->state == TASK_RUNNABLE) {
        // The current task is still runnable. Enqueue into the runqueue.
        enqueue_task(current);
    }

    // Look for the task with the highest priority. Tasks with the same priority
    // is scheduled in round-robin fashion.
    for (int i = 0; i < TASK_PRIORITY_MAX; i++) {
        struct task *next =
            LIST_POP_FRONT(&runqueues[i], struct task, runqueue_next);
        if (next) {
            return next;
        }
    }

    return IDLE_TASK;
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

/// Starts receiving notifications by IRQs.
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

/// Stops receiving notifications by IRQs.
error_t task_unlisten_irq(unsigned irq) {
    if (irq >= IRQ_MAX) {
        return ERR_INVALID_ARG;
    }

    arch_disable_irq(irq);
    irq_owners[irq] = NULL;
    TRACE("disabled IRQ: vector=%d", irq);
    return OK;
}

/// Maps a memory page in the task's virtual memory space. `kpage` is a memory
/// page which provides a memory page for arch-specific page table structures.
///
/// Please note that this is the most DANGEROUS operation in system calls. A user
/// task can map the whole physical memory space including the kernel data area.
__mustuse error_t vm_map(struct task *task, vaddr_t vaddr, paddr_t paddr,
                                paddr_t kpage, unsigned flags) {
    DEBUG_ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(paddr, PAGE_SIZE));
    DEBUG_ASSERT(IS_ALIGNED(kpage, PAGE_SIZE));

    // Prevent corrupting kernel memory.
    if (is_kernel_addr_range(vaddr, PAGE_SIZE)) {
        WARN_DBG("vaddr %p points to a kernel memory area", vaddr);
        return ERR_NOT_ACCEPTABLE;
    }

    // Looks that the mapping is not malicious. Update the page table.
    return arch_vm_map(task, vaddr, paddr, kpage, flags);
}

/// Unmaps a memory page from the task's virtual memory space.
error_t vm_unmap(struct task *task, vaddr_t vaddr) {
    DEBUG_ASSERT(IS_ALIGNED(vaddr, PAGE_SIZE));

    paddr_t paddr = vm_resolve(task, vaddr);
    if (!paddr) {
        return ERR_NOT_FOUND;
    }

    return arch_vm_unmap(task, vaddr);
}

/// Handles timer interrupts. The timer fires this handler every 1/TICK_HZ
/// seconds.
void handle_timer_irq(void) {
    bool resumed_by_timeout = false;
    if (mp_is_bsp()) {
        // Handle task timeouts.
        for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
            struct task *task = &tasks[i];
            if (task->state == TASK_UNUSED || !task->timeout) {
                continue;
            }

            task->timeout--;
            if (!task->timeout) {
                notify(task, NOTIFY_TIMER);
                resumed_by_timeout = true;
            }
        }
    }

    // Switch task if the current task has spend its time slice.
    DEBUG_ASSERT(CURRENT == IDLE_TASK || CURRENT->quantum >= 0);
    CURRENT->quantum--;
    if (CURRENT->quantum < 0 || (CURRENT == IDLE_TASK && resumed_by_timeout)) {
        task_switch();
    }
}

/// Handles interrupts except the timer device used in the kernel.
void handle_irq(unsigned irq) {
    struct task *owner = irq_owners[irq];
    if (owner) {
        notify(owner, NOTIFY_IRQ);
        if (CURRENT == IDLE_TASK) {
            task_switch();
        }
    }
}

/// The page fault handler. It calls a pager and updates the page table.
void handle_page_fault(vaddr_t addr, vaddr_t ip, unsigned fault) {
    ASSERT(CURRENT->pager != NULL);

    struct message m;
    m.type = PAGE_FAULT_MSG;
    m.page_fault.task = CURRENT->tid;
    m.page_fault.vaddr = addr;
    m.page_fault.ip = ip;
    m.page_fault.fault = fault;
    error_t err = ipc(CURRENT->pager, CURRENT->pager->tid,
                      (__user struct message *) &m, IPC_CALL | IPC_KERNEL);
    if (err != OK || m.type != PAGE_FAULT_REPLY_MSG) {
        task_exit(EXP_INVALID_MSG_FROM_PAGER);
    }
}

/// Prints the task states. Used for debugging.
void task_dump(void) {
    const char *states[] = {
        [TASK_UNUSED] = "unused",
        [TASK_RUNNABLE] = "runnable",
        [TASK_BLOCKED] = "blocked",
    };

    for (unsigned i = 0; i < CONFIG_NUM_TASKS; i++) {
        struct task *task = &tasks[i];
        if (task->state == TASK_UNUSED) {
            continue;
        }

        INFO("#%d %s: state=%s, src=%d", task->tid, task->name,
                states[task->state], task->src);
        if (!list_is_empty(&task->senders)) {
            INFO("  senders:");
            LIST_FOR_EACH (sender, &task->senders, struct task, sender_next) {
                INFO("    - #%d %s", sender->tid, sender->name);
            }
        }
    }
}

/// Initializes the task subsystem.
void task_init(void) {
    for (int i = 0; i < TASK_PRIORITY_MAX; i++) {
        list_init(&runqueues[i]);
    }

    for (int i = 0; i < CONFIG_NUM_TASKS; i++) {
        tasks[i].state = TASK_UNUSED;
        tasks[i].tid = i + 1;
    }

    for (int i = 0; i < IRQ_MAX; i++) {
        irq_owners[i] = NULL;
    }
}
