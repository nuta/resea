#include <arch.h>
#include <debug.h>
#include <memory.h>
#include <printk.h>
#include <process.h>
#include <table.h>
#include <thread.h>
#include <timer.h>

/// Creates a new thread. If `process` is `kernel_process`, it creates a kernel
/// thread, which is a special thread which runs only in the kernel land. In
/// that case, `stack` and `user_buffer` are ignored (it uses kernel stack and
/// buffer allocated in this function).
struct thread *thread_create(struct process *process, vaddr_t start,
                             vaddr_t stack, vaddr_t user_buffer,
                             void *arg) {

    int tid = table_alloc(&process_table);
    if (!tid) {
        return NULL;
    }

    struct thread *thread = KMALLOC(&object_arena, sizeof(struct thread));
    if (!thread) {
        return NULL;
    }

    vaddr_t kernel_stack = (vaddr_t) KMALLOC(&page_arena, PAGE_SIZE);
    if (!kernel_stack) {
        kfree(&object_arena, thread);
        return NULL;
    }

    struct thread_info *thread_info = KMALLOC(&page_arena, PAGE_SIZE);
    if (!thread_info) {
        kfree(&object_arena, thread);
        kfree(&page_arena, (void *) kernel_stack);
        return NULL;
    }

    struct message *kernel_ipc_buffer = KMALLOC(&page_arena,
                                                sizeof(struct message));
    if (!kernel_ipc_buffer) {
        kfree(&object_arena, thread);
        kfree(&page_arena, (void *) kernel_stack);
        kfree(&page_arena, thread_info);
        return NULL;
    }

    bool is_kernel_thread = IS_KERNEL_PROCESS(process);
    thread_info->arg = (vaddr_t) arg;
    thread->ipc_buffer = &thread_info->ipc_buffer;
    thread->kernel_ipc_buffer = kernel_ipc_buffer;
    thread->tid = tid;
    thread->state = THREAD_BLOCKED;
    thread->kernel_stack = kernel_stack;
    thread->process = process;
    thread->info = thread_info;
    thread->blocked_on = NULL;
    thread->abort_reason = OK;
#ifdef DEBUG_BUILD
    thread->debug.send_from = NULL;
    thread->debug.receive_from = NULL;
#endif
    init_stack_canary(kernel_stack);

    error_t err = arch_thread_init(thread, start, stack, kernel_stack,
                                   user_buffer, is_kernel_thread);
    if (err != OK) {
        kfree(&object_arena, thread);
        kfree(&page_arena, (void *) kernel_stack);
        kfree(&page_arena, thread_info);
        kfree(&page_arena, kernel_ipc_buffer);
        return NULL;
    }

    // Allow threads to read and write the buffer.
    if (!is_kernel_thread) {
        link_page(&process->page_table, user_buffer, into_paddr(thread_info),
            1, PAGE_USER | PAGE_WRITABLE);
        list_push_back(&process->threads, &thread->next);
    }

    table_set(&process_table, tid, (void *) thread);

#ifdef DEBUG_BUILD
    asan_init_area(ASAN_VALID, thread_info, PAGE_SIZE);
    asan_init_area(ASAN_VALID, (void *) kernel_stack, PAGE_SIZE);
    asan_init_area(ASAN_VALID, (void *) kernel_ipc_buffer, PAGE_SIZE);
#endif

    TRACE("new thread: pid=%d, tid=%d", process->pid, tid);
    return thread;
}

/// Destroys a thread.
void thread_destroy(UNUSED struct thread *thread) {
    if (thread != CURRENT /* TODO: SMP */ && thread->state == THREAD_RUNNABLE) {
        list_remove(&thread->runqueue_next);
    }

    struct channel *blocked_on = thread->blocked_on;
    if (blocked_on) {
        list_remove(&thread->queue_next);
    }

    kfree(&page_arena, (void *) thread->kernel_stack);
    kfree(&page_arena, thread->info);
    kfree(&page_arena, thread->kernel_ipc_buffer);

#ifdef DEBUG_BUILD
    thread->process           = INVALID_POINTER;
    thread->kernel_stack      = (vaddr_t) INVALID_POINTER;
    thread->info              = INVALID_POINTER;
    thread->ipc_buffer        = INVALID_POINTER;
    thread->kernel_ipc_buffer = INVALID_POINTER;
    thread->blocked_on        = INVALID_POINTER;
#endif

    arch_thread_destroy(thread);
    table_free(&process_table, thread->tid);
    list_remove(&thread->next);
    kfree(&object_arena, thread);

    if (thread == CURRENT) {
        UNIMPLEMENTED();
        // TODO: thread_switch_without_save_current();
    }
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
    if (thread->state != THREAD_RUNNABLE) {
        thread->state = THREAD_RUNNABLE;
        list_push_back(&runqueue, &thread->runqueue_next);
    }
}

/// Marks a thread blocked.
void thread_block(struct thread *thread) {
    thread->state = THREAD_BLOCKED;
}

/// Picks the next thread to run.
static struct thread *scheduler(struct thread *current) {
    if (current->state == THREAD_RUNNABLE) {
        list_push_back(&runqueue, &current->runqueue_next);
    }

    struct list_head *next = list_pop_front(&runqueue);
    if (!next) {
        // No runnable threads. Enter the idle thread.
        return CPUVAR->idle_thread;
    }

    return LIST_CONTAINER(next, struct thread, runqueue_next);
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

static void thread_switch_by_timeout(UNUSED struct timer *timer) {
    thread_switch();
}

/// Initializes the thread subsystem.
void thread_init(void) {
    list_init(&runqueue);
    struct thread *idle_thread = thread_create(kernel_process, 0, 0, 0, 0);
    set_current_thread(idle_thread);
    CPUVAR->idle_thread = idle_thread;
    timer_create(THREAD_SWITCH_INTERVAL, THREAD_SWITCH_INTERVAL,
                 thread_switch_by_timeout, NULL);
}
