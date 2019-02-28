#ifndef __PROCESS_H__
#define __PROCESS_H__

#include <types.h>
#include <collections.h>
#include <collections.h>
#include <arch.h>

#define KERNEL_PID 1
#define PROCESS_NAME_LEN_MAX 16

typedef intmax_t cid_t;
typedef intmax_t pid_t;

struct vmarea;
typedef paddr_t (*pager_t)(struct vmarea *vma, vaddr_t vaddr);

struct channel {
    cid_t cid;
    spinlock_t lock;
    int ref_count;
    struct process *process;
    struct channel *linked_to;
    struct channel *transfer_to;
    struct thread *receiver;
    struct list_head sender_queue;
};

struct vmarea {
    struct list_head next;
    vaddr_t vaddr;
    size_t len;
    pager_t pager;
    void *arg;
    uintmax_t flags;
};

struct process {
    struct list_head next;
    pid_t pid;
    char name[PROCESS_NAME_LEN_MAX];
    spinlock_t lock;
    struct list_head vmareas;
    struct list_head threads;
    struct arch_page_table page_table;
    struct idtable channels;
};

extern struct process *kernel_process;
extern struct idtable all_processes;

void process_init(void);
struct process *process_create(const char *name);
void process_destroy(struct process *process);
int vmarea_add(struct process* process,
               vaddr_t vaddr,
               size_t len,
               pager_t pager,
               void *pager_arg,
               int flags);

#endif
