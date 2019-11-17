#include <memory.h>
#include <printk.h>
#include <process.h>
#include <server.h>
#include <table.h>
#include <thread.h>

/// The process/thread table.
struct table process_table;
struct list_head process_list;
/// The kernel process.
struct process *kernel_process;
/// THe first userland process (typically memmgr).
struct process *init_process;

static char *strcpy(char *dst, size_t dst_len, const char *src) {
    DEBUG_ASSERT(dst != NULL && "copy to NULL");
    DEBUG_ASSERT(src != NULL && "copy from NULL");

    size_t i = 0;
    while (i < dst_len - 1 && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
    return dst;
}

/// Creates a new process. `name` is used for just debugging use.
struct process *process_create(const char *name) {
    int pid = table_alloc(&process_table);
    if (!pid) {
        return NULL;
    }

    struct process *proc = KMALLOC(&object_arena, sizeof(struct process));
    if (!proc) {
        table_free(&process_table, pid);
        return NULL;
    }

    if (table_init(&proc->channels) != OK) {
        kfree(&object_arena, proc);
        table_free(&process_table, pid);
        return NULL;
    }

    proc->pid = pid;
    strcpy((char *) &proc->name, PROCESS_NAME_LEN_MAX, name);
    list_init(&proc->vmareas);
    list_init(&proc->threads);
    list_init(&proc->channel_list);
    page_table_init(&proc->page_table);

    table_set(&process_table, pid, (void *) proc);
    list_push_back(&process_list, &proc->next);

    TRACE("new process: pid=%d, name=%s", pid, &proc->name);
    return proc;
}

/// Destroys a process.
void process_destroy(UNUSED struct process *process) {
    if (process == init_process) {
        PANIC("Tried to kill the init process!");
    }

    // Destroy channels.
    LIST_FOR_EACH(ch, &process->channel_list, struct channel, next) {
        channel_destroy(ch);
    }

    // Destroy threads.
    LIST_FOR_EACH(thread, &process->threads, struct thread, next) {
        thread_destroy(thread);
    }

    // Destroy vmareas.
    LIST_FOR_EACH(vmarea, &process->vmareas, struct vmarea, next) {
        vmarea_destroy(vmarea);
    }

    table_destroy(&process->channels);
    // TODO: Send pages back to memmgr.
    page_table_destroy(&process->page_table);
    table_free(&process_table, process->pid);
    list_remove(&process->next);
    kfree(&object_arena, process);

    // TODO: Send a message to its pager server (@1).
}


/// Initializes the process subsystem.
void process_init(void) {
    list_init(&process_list);
    if (table_init(&process_table) != OK) {
        PANIC("failed to initialize process_table");
    }

    kernel_process = process_create("kernel");
    ASSERT(kernel_process->pid == KERNEL_PID);
}
