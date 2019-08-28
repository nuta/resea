#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <arch.h>
#include <list.h>
#include <table.h>

#define KERNEL_PID 1
#define PROCESS_NAME_LEN_MAX 16

struct process {
    pid_t pid;
    char name[PROCESS_NAME_LEN_MAX];
    spinlock_t lock;
    struct list_head vmareas;
    struct list_head threads;
    struct arch_page_table page_table;
    struct idtable channels;
};

struct vmarea;
typedef paddr_t (*pager_t)(struct vmarea *vma, vaddr_t vaddr);

struct vmarea {
    struct list_head next;
    vaddr_t start;
    vaddr_t end;
    pager_t pager;
    void *arg;
    uintmax_t flags;
};

struct channel {
    cid_t cid;
    spinlock_t lock;
    int ref_count;
    struct process *process;
    struct channel *linked_to;
    struct channel *transfer_to;
    struct thread *receiver;
    struct list_head queue;
};

extern struct process *kernel_process;
extern struct idtable all_processes;

void process_init(void);
struct process *process_create(const char *name);
void process_destroy(struct process *process);
error_t vmarea_add(struct process *process, vaddr_t start, vaddr_t end,
                   pager_t pager, void *pager_arg, int flags);

#endif
