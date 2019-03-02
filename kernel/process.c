#include <process.h>
#include <memory.h>
#include <printk.h>
#include <collections.h>

// Processes and threads share this id space.
struct idtable all_processes;
struct process *kernel_process;

static struct process *process_create_with_pid(const char *name, pid_t pid) {
    struct process *proc = alloc_object();
    if (!proc) {
        return NULL;
    }

    if (!idtable_init(&proc->channels)) {
        free_object(proc);
        return NULL;
    }

    proc->pid = pid;
    strcpy((char *) &proc->name, PROCESS_NAME_LEN_MAX, name);
    spin_lock_init(&proc->lock);
    list_init(&proc->vmareas);
    list_init(&proc->threads);
    arch_page_table_init(&proc->page_table);

    idtable_set(&all_processes, pid, (void *) proc);

    DEBUG("new process: pid=%d, name=%s", pid, &proc->name);
    return proc;
}

struct process *process_create(const char *name) {
    int pid = idtable_alloc(&all_processes);
    if (!pid) {
        return NULL;
    }

    struct process *proc = process_create_with_pid(name, pid);
    if (!proc) {
        idtable_free(&all_processes, pid);
        return NULL;
    }

    return proc;
}

void process_destroy(UNUSED struct process *process) {
    // TODO:
    UNIMPLEMENTED();
}

// Returns 0 on failure.
int vmarea_add(struct process* process,
               vaddr_t start,
               vaddr_t end,
               pager_t pager,
               void *pager_arg,
               int flags) {

    struct vmarea *vma = alloc_object();
    if (!vma) {
        return 0;
    }

    DEBUG("new vmarea: vaddr=%p-%p", start, end);
    vma->start = start;
    vma->end   = end;
    vma->pager = pager;
    vma->arg   = pager_arg;
    vma->flags = flags;

    list_push_back(&process->vmareas, &vma->next);
    return 1;
}

void process_init(void) {
    if (!idtable_init(&all_processes)) {
        PANIC("failed to initialize all_processes");
    }

    if (idtable_alloc(&all_processes) != KERNEL_PID) {
        PANIC("failed to reserve KERNEL_PID");
    }

    kernel_process = process_create_with_pid("kernel", KERNEL_PID);
}
