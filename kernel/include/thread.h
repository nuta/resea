#ifndef __THREAD_H__
#define __THREAD_H__

#include <arch.h>
#include <ipc.h>
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
    struct message ipc_buffer;
} PACKED;

struct process;

/// The thread control block.
struct thread {
    /// Architecture-dependent context such as registers.
    struct arch_thread arch;
    /// The thread ID.
    tid_t tid;
    /// The thread lock.
    spinlock_t lock;
    /// The process.
    struct process *process;
    /// The current state of the thread.
    int state;
    /// True if this thread is waiting for a message and the sys_ipc() is
    /// invoked from the kernel (e.g. calling fill_request from tge page fault
    /// handler). If the receiver is kernel, we copy page payloads in physical
    /// addresses.
    bool recv_in_kernel;
    /// The thread-local information block.
    struct thread_info *info;
    /// The current thread-local IPC buffer. This points to whether
    /// info->ipc_buffer or kernel_ipc_buffer.
    struct message *ipc_buffer;
    /// The thread-local kernel IPC buffer. The kernel use this buffer when it
    /// sends a message on behalf of the thread (e.g. pager.fill_message).
    ///
    /// Consider the following example:
    ///
    /// ```
    ///    ipc_buffer->header = PRINTCHAR_HEADER;
    ///    ipc_buffer->data[0] = foo(); // Page fault occurs here!
    ///    // If kernel and user share one ipc_buffer, the buffer is
    ///    // overwritten by the page fault handler. Consequently,
    ///    // ipc_buffer->header is no longer equal to PRINTCHAR_HEADER!
    ///    //
    ///    // This is why we need a kernel's own ipc buffer.
    ///    sys_ipc(ch);
    /// ```
    struct message *kernel_ipc_buffer;
    /// The beginning of (bottom of) dedicated kernel stack.
    vaddr_t kernel_stack;
    /// The entry in a runqueue.
    struct list_head runqueue_elem;
    /// The entry in a message sender queue.
    struct list_head queue_elem;
    /// Next thread in the process.
    struct list_head next;
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
