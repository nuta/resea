#include <memory.h>
#include <printk.h>
#include <process.h>
#include <server.h>
#include <table.h>
#include <thread.h>

/// The process/thread idtable.
struct idtable all_processes;
/// The kernel process.
struct process *kernel_process;

/// Creates a new process. `name` is used for just debugging use.
struct process *process_create(const char *name) {
    int pid = idtable_alloc(&all_processes);
    if (!pid) {
        return NULL;
    }

    struct process *proc = kmalloc(&page_arena);
    if (!proc) {
        idtable_free(&all_processes, pid);
        return NULL;
    }

    if (!idtable_init(&proc->channels)) {
        kfree(&small_arena, proc);
        idtable_free(&all_processes, pid);
        return NULL;
    }

    proc->pid = pid;
    strcpy((char *) &proc->name, PROCESS_NAME_LEN_MAX, name);
    spin_lock_init(&proc->lock);
    list_init(&proc->vmareas);
    list_init(&proc->threads);
    arch_page_table_init(&proc->page_table);

    idtable_set(&all_processes, pid, (void *) proc);

    TRACE("new process: pid=%d, name=%s", pid, &proc->name);
    return proc;
}

/// Destroys a process.
void process_destroy(UNUSED struct process *process) {
    UNIMPLEMENTED();
}

// Adds a new vm area.
error_t vmarea_add(struct process *process, vaddr_t start, vaddr_t end,
                   pager_t pager, void *pager_arg, int flags) {

    struct vmarea *vma = kmalloc(&page_arena);
    if (!vma) {
        return ERR_OUT_OF_MEMORY;
    }

    TRACE("new vmarea: vaddr=%p-%p", start, end);
    vma->start = start;
    vma->end = end;
    vma->pager = pager;
    vma->arg = pager_arg;
    vma->flags = flags;

    list_push_back(&process->vmareas, &vma->next);
    return OK;
}

/// Initializes the process subsystem.
void process_init(void) {
    if (!idtable_init(&all_processes)) {
        PANIC("failed to initialize all_processes");
    }

    kernel_process = process_create("kernel");
    ASSERT(kernel_process->pid == KERNEL_PID);
}
