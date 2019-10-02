#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <arch.h>
#include <list.h>
#include <table.h>
#include <message.h>

#define KERNEL_PID 1
#define PROCESS_NAME_LEN_MAX 32
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

extern struct process *kernel_process;
extern struct process *init_process;
extern struct table process_table;
extern struct list_head process_list;

void process_init(void);
struct process *process_create(const char *name);
void process_destroy(struct process *process);

#endif
