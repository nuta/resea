#include <resea/malloc.h>
#include <resea/ipc.h>
#include <resea/task.h>
#include <string.h>
#include <list.h>
#include "abi.h"
#include "proc.h"
#include "elf.h"
#include "fs.h"
#include "mm.h"
#include "syscall.h"
#include "waitqueue.h"

struct proc *init_proc = NULL;
static pid_t next_pid = 1;
static pid_t next_tid = 16; // FIXME:
static list_t procs;

static task_t alloc_task(void) {
    return next_tid++;
}

static void free_task(task_t tid) {
    // TODO:
}

struct proc *proc_alloc(void) {
    struct proc *proc = malloc(sizeof(*proc));
    proc->state = PROC_ALLOCED;
    proc->pid = next_pid++;
    proc->task = alloc_task();
    list_push_back(&procs, &proc->next);
    return proc;
}

struct proc *proc_lookup_by_task(task_t task) {
    LIST_FOR_EACH(proc, &procs, struct proc, next) {
        if (proc->task == task) {
            return proc;
        }
    }

    return NULL;
}

struct proc *proc_lookup_by_pid(pid_t pid) {
    LIST_FOR_EACH(proc, &procs, struct proc, next) {
        if (proc->pid == pid) {
            return proc;
        }
    }

    return NULL;
}

struct file *get_file_by_fd(struct proc *proc, fd_t fd) {
    if (fd >= FD_MAX) {
        return NULL;
    }

    return proc->files[fd];
}

static void add_aux_vector(uintptr_t **sp, int type, uintptr_t ptr) {
    *(*sp)++ = type;
    *(*sp)++ = ptr;
}

/// Initializes a user stack. See "Initial Process Stack" in System V ABI spec
/// for details.
static errno_t init_stack(struct proc *proc, char *argv[], UNUSED char *envp[]) {
    uintptr_t *sp = proc->stack->buf + STACK_TOP;
    char *strbuf = proc->stack->buf + STACK_STRBUF;
    vaddr_t strbuf_top = (vaddr_t) proc->stack->buf + STACK_STRBUF;
    loff_t stroff = 0;
    size_t strbuf_size = STACK_LEN - STACK_STRBUF;

    // Fill argc.
    int argc = 0;
    while (argv[argc] != NULL) {
        argc++;
    }
    *sp++ = argc;

    // Fill argv.
    for (int i = 0; i < argc; i++) {
        size_t len = strlen(argv[i]);
        *sp++ = proc->stack->vaddr + STACK_STRBUF + stroff;

        if (stroff + len + 1 >= strbuf_size) {
            // Too long strings.
            return -ENOMEM;
        }

        if ((vaddr_t) sp >= strbuf_top) {
            // Too many entries.
            return -ENOMEM;
        }

        strncpy(&strbuf[stroff], argv[i], len + 1);
        stroff += len + 1;
    }
    *sp++ = 0;

    // TODO: Fill envp
    *sp++ = 0;

    // Prepare random bytes for AT_RANDOM.
    uint8_t rand_bytes[16]; // TODO:
    if (stroff + sizeof(rand_bytes) >= strbuf_size) {
        return -ENOMEM;
    }

    vaddr_t rand_ptr = proc->stack->vaddr + STACK_STRBUF + stroff;
    memcpy(&strbuf[stroff], rand_bytes, sizeof(rand_bytes));
    stroff += sizeof(rand_bytes);

    // Fill auxiliary vectors.
    add_aux_vector(&sp, AT_PHDR, proc->file_header->vaddr + proc->ehdr->e_phoff);
    add_aux_vector(&sp, AT_PHENT, proc->ehdr->e_phentsize);
    add_aux_vector(&sp, AT_PHNUM, proc->ehdr->e_phnum);
    add_aux_vector(&sp, AT_PAGESZ, PAGE_SIZE);
    add_aux_vector(&sp, AT_RANDOM, rand_ptr);
    add_aux_vector(&sp, AT_NULL, 0);

    return 0;
}

errno_t proc_execve(struct proc *proc, const char *path, char *argv[],
                    char *envp[]) {
    // TODO: free resouces on failures

    // Clear the address space.
    // FIXME: Move into mm.
    list_init(&proc->mm.mchunks);
    DEBUG_ASSERT(list_is_empty(&proc->mm.mchunks));

    task_destroy(proc->task);
    free_task(proc->task);
    proc->task = alloc_task();
    error_t e = task_create(proc->task, proc->name, 0, task_self(), TASK_ABI_EMU);
    if (e != OK) {
        return -EAGAIN;
    }

    // Open the executable file.
    struct file *file;
    int err = fs_open(&file, path, 0, 0);
    if (err < 0) {
        return err;
    }
    proc->exec = file;

    // Read the beginning of the executable to read ELF headers.
    proc->file_header = mm_alloc_mchunk(&proc->mm, ELF_HEADER_ADDR, ELF_HEADER_LEN);
    fs_read_pos(proc->exec, proc->file_header->buf, 0, PAGE_SIZE);

    // Ensure it's an ELF file.
    struct elf64_ehdr *ehdr = (struct elf64_ehdr *) proc->file_header->buf;
    if (memcmp(ehdr->e_ident, "\x7f" "ELF", 4) != 0) {
        return -ENOEXEC;
    }

    // Fill out fields.
    ASSERT(argv[0]);
    strncpy(proc->name, argv[0], sizeof(proc->name));
    proc->state = PROC_RUNNABLE;
    proc->current_brk = HEAP_ADDR;
    proc->ehdr = ehdr;
    proc->phdrs = (struct elf64_phdr *) ((uintptr_t) ehdr + ehdr->e_ehsize);
    proc->fsbase = 0xbad0ffff;
    proc->gsbase = 0xbad0eeee;

    // Initialize the stack.
    struct mchunk *stack_mchunk = mm_alloc_mchunk(&proc->mm, STACK_ADDR, STACK_LEN);
    if (!stack_mchunk) {
        return -ENOMEM;
    }

    proc->stack = stack_mchunk;
    if ((err = init_stack(proc, argv, envp)) < 0) {
        return err;
    }

    return 0;
}

pid_t proc_fork(struct proc *parent) {
    struct proc *child = proc_alloc();
    if (!child) {
        return -EAGAIN;
    }

    fs_fork(parent, child);
    mm_fork(parent, child);

    child->parent = parent;
    if (parent) {
        child->state = PROC_FORKED;
        strncpy(child->name, parent->name, sizeof(child->name));
        child->exec = parent->exec;
        child->current_brk = parent->current_brk;
        child->ehdr = parent->ehdr;
        child->phdrs = parent->phdrs;
        child->fsbase = parent->fsbase;
        child->gsbase = parent->gsbase;
        child->file_header = mm_clone_mchunk(child, parent->file_header);
        child->stack = mm_clone_mchunk(child, parent->stack);
        memcpy(&child->frame, &parent->frame, sizeof(child->frame));
    } else {
        // The init process.
        ASSERT(child->pid == 1);
        strncpy(child->name, "init", sizeof(child->name));
        child->state = PROC_RUNNABLE;
        child->exec = NULL;
        child->current_brk = 0;
        child->file_header = NULL;
        child->ehdr = NULL;
        child->phdrs = NULL;
        child->fsbase = 0;
        child->gsbase = 0;
    }

    error_t err = task_create(child->task, child->name, 0, task_self(),
                              TASK_ABI_EMU);
    if (err != OK) {
        return -EAGAIN;
    }

    return child->pid;
}

void proc_block(struct proc *proc, struct waitqueue *wq) {
    DEBUG_ASSERT(proc->state != PROC_SLEEPING);
    proc->state = PROC_SLEEPING;
    waitqueue_add(wq, proc);
}

void proc_resume(struct proc *proc) {
    DEBUG_ASSERT(proc->state == PROC_SLEEPING);
    proc->state = PROC_RUNNABLE;
    try_syscall(proc);
}

void proc_destroy(struct proc *proc) {
    // TODO:
    list_remove(&proc->next);
}

struct waitqueue waiting_procs_wq;

pid_t proc_try_wait(struct proc *proc, pid_t pid, int *wstatus, int options) {
    LIST_FOR_EACH(child, &procs, struct proc, next) {
        // TODO: use pid
        if (child->state == PROC_EXITED && child->parent == proc) {
            // TODO: return status
            pid_t ret = child->pid;
            proc_destroy(child);
            return ret;
        }
    }

    return -EAGAIN;
}

void proc_exit(struct proc *proc) {
    proc->state = PROC_EXITED;
    waitqueue_wake_all(&waiting_procs_wq);
}

void proc_init(void) {
    list_init(&procs);
    waitqueue_init(&waiting_procs_wq);
    proc_fork(NULL);
    init_proc = proc_lookup_by_pid(1);
    ASSERT(init_proc);
}
