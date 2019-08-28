#include <arch.h>
#include <debug.h>
#include <memory.h>
#include <printk.h>
#include <process.h>
#include <table.h>
#include <thread.h>

/// Creates a new thread.
struct thread *thread_create(struct process *process, uintmax_t start,
                             uintmax_t stack, vaddr_t user_buffer,
                             uintmax_t arg) {

    int tid = idtable_alloc(&all_processes);
    if (!tid) {
        return NULL;
    }

    struct thread *thread = kmalloc(&page_arena);
    if (!thread) {
        return NULL;
    }

    vaddr_t kernel_stack = (vaddr_t) kmalloc(&page_arena);
    if (!kernel_stack) {
        kfree(&small_arena, thread);
        return NULL;
    }

    struct thread_info *thread_info =
        (struct thread_info *) kmalloc(&page_arena);
    if (!thread_info) {
        kfree(&small_arena, thread);
        kfree(&page_arena, (void *) kernel_stack);
        return NULL;
    }

    thread_info->arg = arg;
    thread->tid = tid;
    thread->state = THREAD_BLOCKED;
    thread->kernel_stack = kernel_stack;
    thread->process = process;
    thread->info = thread_info;
    spin_lock_init(&thread->lock);
    init_stack_canary(kernel_stack);
    arch_thread_init(thread, start, stack, kernel_stack, user_buffer);

    // Allow threads to read and write the buffer.
    flags_t flags = spin_lock_irqsave(&process->lock);
    arch_link_page(&process->page_table, user_buffer, into_paddr(thread_info),
        1, PAGE_USER | PAGE_WRITABLE);
    list_push_back(&process->threads, &thread->next);
    spin_unlock_irqrestore(&process->lock, flags);

    idtable_set(&all_processes, tid, (void *) thread);

    TRACE("new thread: pid=%d, tid=%d", process->pid, tid);

    if (!thread) {
        idtable_free(&all_processes, tid);
        return NULL;
    }

    return thread;
}

/// Destroys a thread.
void thread_destroy(UNUSED struct thread *thread) {
    UNIMPLEMENTED();
}

/// Kills the current thread and switches into another thread. This function
/// won't return.
NORETURN void thread_kill_current(void) {
    thread_destroy(CURRENT);
    UNREACHABLE;
}

static struct list_head runqueue;

/// Marks a thread runnable.
void thread_resume(struct thread *thread) {
    flags_t flags = spin_lock_irqsave(&thread->lock);

    if (thread->state != THREAD_RUNNABLE) {
        thread->state = THREAD_RUNNABLE;
        list_push_back(&runqueue, &thread->runqueue_elem);
    }

    spin_unlock_irqrestore(&thread->lock, flags);
}

/// Marks a thread blocked.
void thread_block(struct thread *thread) {
    flags_t flags = spin_lock_irqsave(&thread->lock);
    thread->state = THREAD_BLOCKED;
    spin_unlock_irqrestore(&thread->lock, flags);
}

/// Picks the next thread to run.
static struct thread *scheduler(struct thread *current) {
    if (current->state == THREAD_RUNNABLE) {
        list_push_back(&runqueue, &current->runqueue_elem);
    }

    struct list_head *next = list_pop_front(&runqueue);
    if (!next) {
        // No runnable threads. Enter the idle thread.
        return CPUVAR->idle_thread;
    }

    return LIST_CONTAINER(thread, runqueue_elem, next);
}

/// Saves the current context and switches into the next thread. This function
/// will return when another thread resumed the current thread.
void thread_switch(void) {
    check_stack_canary();

    struct thread *current = CURRENT;
    struct thread *next = scheduler(current);
    if (next == current) {
        // No runnable threads other than the current one. Continue executing
        // the current thread.
        return;
    }

    // Switch into the next thread. This function will return when another
    // thread switches into the current one.
    arch_thread_switch(CURRENT, next);

    check_stack_canary();
}

/// Initializes the thread subsystem.
void thread_init(void) {
    list_init(&runqueue);
    struct thread *idle_thread = thread_create(kernel_process, 0, 0, 0, 0);
    arch_set_current_thread(idle_thread);
    CPUVAR->idle_thread = idle_thread;
}
