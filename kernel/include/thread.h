#ifndef __THREAD_H__
#define __THREAD_H__

#include <arch.h>
#include <ipc.h>
#include <list.h>
#include <types.h>

#define THREAD_BLOCKED 0
#define THREAD_RUNNABLE 1
#define KERNEL_STACK_SIZE PAGE_SIZE
#define CURRENT arch_get_current_thread()

// Thread-local information block. Keep in mind that this block is readable and
// WRITABLE from the user: don't put sensitive data in it.
struct thread_info {
    vaddr_t arg;
    struct message ipc_buffer;
} PACKED;

struct process;
struct thread {
    // Architecture-dependent context such as registers.
    struct arch_thread arch;
    // The thread-local information block.
    struct thread_info *info;
    // The current thread-local IPC buffer. This points to whether
    // info->ipc_buffer or kernel_ipc_buffer.
    struct message *ipc_buffer;
    // The thread-local kernel IPC buffer. The kernel use this buffer when it
    // sends a message on behalf of the thread (e.g. pager.fill_message).
    //
    // Here's reason why we need it. Consider the following program in userland:
    //
    //    ipc_buffer->header = PRINTCHAR_HEADER;
    //    ipc_buffer->data[0] = foo();
    //    /* Page fault occurs in foo(). */
    //    sys_ipc(ch);
    //
    // If kernel and user share one ipc_buffer, sys_ipc() will send a reply
    // message of pager.fill_request, not runtime.printchar because the page
    // fault handler will overwrite the buffer to call the pager.
    //
    struct message *kernel_ipc_buffer;
    // The thread ID.
    tid_t tid;
    // The thread lock.
    spinlock_t lock;
    // The process.
    struct process *process;
    // The current state of the thread.
    int state;
    //
    bool recv_in_kernel;
    // The beginning of (bottom of) dedicated kernel stack.
    vaddr_t kernel_stack;
    // List elements.
    struct list_head runqueue_elem;
    struct list_head queue_elem;
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
