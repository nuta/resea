#ifndef __THREAD_H__
#define __THREAD_H__

#include <types.h>
#include <process.h>
#include <collections.h>
#include <ipc.h>
#include <arch.h>

#define THREAD_BLOCKED  0
#define THREAD_RUNNABLE 1
#define THREAD_RECV_BY_KERNEL  2

typedef intmax_t tid_t;

struct thread {
    // Architecture-dependent context such as registers.
    struct arch_thread arch;
    // The thread ID.
    tid_t tid;
    // The thread lock.
    spinlock_t lock;
    // The process.
    struct process *process;
    // The current state of the thread.
    int state;
    // The beginning of (bottom of) dedicated kernel stack.
    vaddr_t kernel_stack;

    payload_t header;
    payload_t buf[6];
    int recved_by_kernel;

    // List elements.
    struct list_head runqueue_elem;
    struct list_head sender_queue_elem;
    struct list_head next;
};

void thread_init(void);
void thread_switch(void);
struct thread *thread_create(struct process * process,
                             uintmax_t start,
                             uintmax_t stack,
                             uintmax_t arg);
void thread_block(struct thread *thread);
void thread_resume(struct thread *thread);
NORETURN void thread_kill_current(void);

#endif
