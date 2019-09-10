#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <arch.h>
#include <list.h>
#include <table.h>

#define KERNEL_PID 1
#define PROCESS_NAME_LEN_MAX 16
#define IS_KERNEL_PROCESS(proc) ((proc) == kernel_process)

/// The process control block.
struct process {
    /// The process lock.
    spinlock_t lock;
    /// The process ID.
    pid_t pid;
    /// The process name for humans. The kernel does not care about the
    /// content,
    char name[PROCESS_NAME_LEN_MAX];
    /// Regions in the virtual address space.
    struct list_head vmareas;
    /// Threads in the process.
    struct list_head threads;
    /// The page table.
    struct page_table page_table;
    /// Channels in the process.
    struct table channels;
};

struct vmarea;
typedef paddr_t (*pager_t)(struct vmarea *vma, vaddr_t vaddr);

/// A region in a virtual address space.
struct vmarea {
    /// The next vmarea in the process.
    struct list_head next;
    /// The start of the virtual memory region.
    vaddr_t start;
    /// The start of the virtual memory region.
    vaddr_t end;
    /// The pager function. A pager is responsible to fill pages within the
    /// vmarea and is called when a page fault has occurred.
    pager_t pager;
    /// The pager-specific data.
    void *arg;
    /// Allowed operations to the vmarea. Defined operations are:
    ///
    /// - PAGE_WRITABLE: The region is writable. If this flag is not set, the
    ///                  region is readonly.
    /// - PAGE_USER: The region is accessible from the user. If this flag is
    ///              not set, the region is for accessible only from the
    ///              kernel.
    ///
    uintmax_t flags;
};

/// The channel control block.
struct channel {
    /// The owner process.
    struct process *process;
    /// The channel lock.
    spinlock_t lock;
    /// The channel ID.
    cid_t cid;
    /// The reference counter. Even if the owner process has been destroyed,
    /// some of its channels must be alive to prevent dangling pointers until
    /// no one references them.
    int ref_count;
    /// If set, `notification` contains a noitification data.
    bool notified;
    /// The received notification data.
    intmax_t notification;
    /// The channel linked with this channel. The messages sent from this
    /// channel will be sent to this channel. The destination channel can be
    /// channels in another process.
    ///
    /// By default, this points to the itself (i.e. `ch->linked_to== ch`).
    struct channel *linked_to;
    /// The transfer channel (FIXME: we need more appropriate name). The
    /// messages sent from the channel linked with this channel will be
    /// transferred to the transfer channel.
    ///
    /// By default, this points to the itself (i.e. `ch->transfer_to == ch`).
    struct channel *transfer_to;
    /// The receiver thread. This is NULL if no threads are in the receive
    /// phase on this channel.
    struct thread *receiver;
    /// The sender thread queue.
    struct list_head queue;
};

extern struct process *kernel_process;
extern struct table all_processes;

void process_init(void);
struct process *process_create(const char *name);
void process_destroy(struct process *process);
error_t vmarea_add(struct process *process, vaddr_t start, vaddr_t end,
                   pager_t pager, void *pager_arg, int flags);

#endif
