#ifndef __THREAD_H__
#define __THREAD_H__

#include <arch.h>
#include <message.h>
#include <list.h>
#include <types.h>

#define KERNEL_STACK_SIZE PAGE_SIZE
#define CURRENT get_current_thread()

struct process;

/// The thread control block.
struct thread {
    /// Architecture-dependent context such as registers.
    struct arch_thread arch;
    /// The beginning of (bottom of) dedicated kernel stack.
    vaddr_t kernel_stack;

    /// The process to which the thread belongs.
    struct process *process;
    /// Next thread in the process.
    struct list_head next;
    /// The thread ID.
    tid_t tid;

    /// The thread state. It's true if the thread is blocked. Note that blocking
    /// occurrs only within `sys_ipc()`.
    bool blocked;
    /// It is not `OK` if an IPC operation is aborted due to channel
    /// destruction, etc.
    error_t abort_reason;
    /// The channel at which the thread is trying to send/receive a message.
    struct channel *blocked_on;

    /// The thread-local information block.
    struct thread_info *info;
    /// The current thread-local IPC buffer. This points to whether
    /// info->ipc_buffer or kernel_ipc_buffer.
    struct message *ipc_buffer;
    /// The thread-local kernel IPC buffer. The kernel use this buffer when it
    /// sends a message on behalf of the thread (e.g. pager.fill_request).
    struct message *kernel_ipc_buffer;

    /// The entry in a runqueue.
    ///
    /// TODO: I belive it is safe to add an union to hold runqueue_next and
    ///       queue_next.
    struct list_head runqueue_next;
    /// The entry in a message sender queue.
    struct list_head queue_next;

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
void thread_first_switch(void);
struct thread *thread_create(struct process *process, vaddr_t start,
                             vaddr_t stack, vaddr_t user_buffer,
                             void *arg);
void thread_destroy(struct thread *thread);
void thread_block(struct thread *thread);
void thread_resume(struct thread *thread);
NORETURN void thread_kill_current(void);

#endif
