#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <arch.h>
#include <list.h>
#include <table.h>
#include <message.h>

#define KERNEL_PID 1
#define PROCESS_NAME_LEN_MAX 16
#define IS_KERNEL_PROCESS(proc) ((proc) == kernel_process)

/// The process control block.
struct process {
    /// The list element used for `process_list`.
    struct list_head next;
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
    /// Channels in the table.
    struct list_head channel_list;
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

extern struct process *kernel_process;
extern struct table process_table;
extern struct list_head process_list;

void process_init(void);
struct process *process_create(const char *name);
void process_destroy(struct process *process);
error_t vmarea_add(struct process *process, vaddr_t start, vaddr_t end,
                   pager_t pager, void *pager_arg, int flags);

#endif
