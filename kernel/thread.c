#include <arch.h>
#include <thread.h>
#include <process.h>
#include <memory.h>
#include <printk.h>
#include <collections.h>

static struct thread *thread_create_with_pid(struct process *process,
                                             uintmax_t start,
                                             uintmax_t stack,
                                             vaddr_t user_buffer,
                                             uintmax_t arg,
                                             pid_t tid) {
    struct thread *thread = alloc_object();
    if (!thread) {
        return NULL;
    }

    vaddr_t kernel_stack = (vaddr_t) alloc_page();
    if (!kernel_stack) {
        free_object(thread);
        return NULL;
    }

    struct message *buffer = (struct message *) alloc_page();
    if (!buffer) {
        free_object(thread);
        free_page((void *) kernel_stack);
        return NULL;
    }

    struct arch_thread_info *thread_info = (struct arch_thread_info *) alloc_page();
    if (!buffer) {
        free_object(thread);
        free_page((void *) kernel_stack);
        free_page((void *) buffer);
        return NULL;
    }

    thread->tid = tid;
    thread->state = THREAD_BLOCKED;
    thread->kernel_stack = kernel_stack;
    thread->process = process;
    thread->buffer = buffer;
    thread->info = thread_info;
    spin_lock_init(&thread->lock);
    write_stack_canary(kernel_stack);
    arch_thread_init(thread, start, stack, kernel_stack, user_buffer, arg);

    // Allow threads to read and write the buffer.
    flags_t flags = spin_lock_irqsave(&process->lock);
    arch_link_page(&process->page_table, user_buffer, into_paddr(thread_info), 1,
        PAGE_USER);
    arch_link_page(&process->page_table, user_buffer + PAGE_SIZE, into_paddr(buffer), 1,
        PAGE_USER | PAGE_WRITABLE);
    list_push_back(&process->threads, &thread->next);
    spin_unlock_irqrestore(&process->lock, flags);

    idtable_set(&all_processes, tid, (void *) thread);

    DEBUG("new thread: pid=%d, tid=%d", process->pid, tid);
    return thread;
}

struct thread *thread_create(struct process * process,
                             uintmax_t start,
                             uintmax_t stack,
                             vaddr_t user_buffer,
                             uintmax_t arg) {

    int tid = idtable_alloc(&all_processes);
    if (!tid) {
        return NULL;
    }

    struct thread *thread = thread_create_with_pid(process, start, stack,
        user_buffer, arg, tid);

    if (!thread) {
        idtable_free(&all_processes, tid);
        return NULL;
    }

    return thread;
}

struct list_head runqueue;

void thread_resume(struct thread *thread) {
    flags_t flags = spin_lock_irqsave(&thread->lock);

    if (thread->state != THREAD_RUNNABLE) {
        thread->state = THREAD_RUNNABLE;
        list_push_back(&runqueue, &thread->runqueue_elem);
    }

    spin_unlock_irqrestore(&thread->lock, flags);
}

void thread_block(struct thread *thread) {
    flags_t flags = spin_lock_irqsave(&thread->lock);
    thread->state = THREAD_BLOCKED;
    spin_unlock_irqrestore(&thread->lock, flags);
}

NORETURN void thread_kill_current(void) {
    // TODO:
    UNIMPLEMENTED();
}

struct thread *scheduler(void) {
    struct list_head *next = list_pop_front(&runqueue);
    if (!next) {
        OOPS("resuming idle!");
        return CPUVAR->idle_thread;
    }

    return LIST_CONTAINER(thread, runqueue_elem, next);
}

void thread_switch(void) {
    check_stack_canary();

    if (CURRENT->state == THREAD_RUNNABLE && list_is_empty(&runqueue)) {
        // No runnable threads other than the current one. Continue executing
        // the current thread.
        return;
    }

    struct thread *next = scheduler();
    if (CURRENT->state == THREAD_RUNNABLE) {
        // The current thread has spent its quantum. Enqueue it into the
        // runqueue again to resume later.
        list_push_back(&runqueue, &CURRENT->runqueue_elem);
    }

    arch_thread_switch(CURRENT, next);

    // Now we have returned from another threads.
    check_stack_canary();
}

void thread_init(void) {
    list_init(&runqueue);
    struct thread *idle_thread = thread_create(kernel_process, 0, 0, 0, 0);
    arch_set_current_thread(idle_thread);
    CPUVAR->idle_thread = idle_thread;
}
