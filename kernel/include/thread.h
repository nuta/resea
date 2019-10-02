#ifndef __THREAD_H__
#define __THREAD_H__

#include <arch.h>
#include <message.h>
#include <list.h>
#include <types.h>

#define THREAD_BLOCKED 0
#define THREAD_RUNNABLE 1
#define KERNEL_STACK_SIZE PAGE_SIZE
#define CURRENT get_current_thread()

/// Thread-local information block. Keep in mind that this block is readable and
/// WRITABLE from the user: don't put sensitive data in it.
struct thread_info {
    vaddr_t arg;
    vaddr_t page_base;
    struct message ipc_buffer;
} PACKED;

struct process;

/// The thread control block.
struct thread {
    /// Architecture-dependent context such as registers.
    struct arch_thread arch;
    /// The thread ID.
    tid_t tid;
    /// The process.
    struct process *process;
    /// The current state of the thread.
    int state;
    /// It is set if a IPC operation is aborted due to channel destruction,
    /// etc.
    error_t abort_reason;
    /// The thread-local information block.
    struct thread_info *info;
    /// The current thread-local IPC buffer. This points to whether
    /// info->ipc_buffer or kernel_ipc_buffer.
    struct message *ipc_buffer;
    /// The thread-local kernel IPC buffer. The kernel use this buffer when it
    /// sends a message on behalf of the thread (e.g. pager.fill_request).
    struct message *kernel_ipc_buffer;
    /// The beginning of (bottom of) dedicated kernel stack.
    vaddr_t kernel_stack;
    /// The entry in a runqueue.
    struct list_head runqueue_next;
    /// The entry in a message sender queue.
    struct list_head queue_next;
    /// The channel at which the thread is trying to send/receive a message.
    struct channel *blocked_on;
    /// Next thread in the process.
    struct list_head next;
#ifdef DEBUG_BUILD
    /// Fields used for debugging.
    struct {
        /// The channel at which the thread is being blocked in the IPC send
        /// phase. It can be NULL.
        struct channel *send_from;
        /// The channel at which the thread is being blocked in the IPC receive
        /// phase. It can be NULL.
        struct channel *receive_from;
    } debug;
#endif
};

void thread_init(void);
void thread_switch(void);
struct thread *thread_create(struct process *process, vaddr_t start,
                             vaddr_t stack, vaddr_t user_buffer,
                             void *arg);
void thread_destroy(struct thread *thread);
void thread_block(struct thread *thread);
void thread_resume(struct thread *thread);
NORETURN void thread_kill_current(void);

#endif
