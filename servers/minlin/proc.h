#ifndef __PROC_H__
#define __PROC_H__

#include <list.h>
#include <resea/ipc.h>
#include "abi.h"
#include "mm.h"

#define FD_MAX  128

enum proc_state {
    PROC_ALLOCED,
    PROC_FORKED,
    PROC_RUNNABLE,
    PROC_SLEEPING,
    PROC_EXITED,
};

struct syscall_context {
    int err;
    long (*handler)(struct proc *proc);
    union {
        struct {
            vaddr_t path;
            int flags;
            mode_t mode;
        } open;
        struct {
            fd_t fd;
        } close;
        struct {
            vaddr_t path;
            vaddr_t stat;
        } stat;
        struct {
            struct file *file;
            size_t len;
            vaddr_t buf;
        } read;
        struct {
            struct file *file;
            size_t len;
            vaddr_t buf;
        } write;
        struct {
            struct file *file;
            vaddr_t iovbase;
            int iovcnt;
        } writev;
        struct {
            vaddr_t path;
            vaddr_t argv;
            vaddr_t envp;
        } execve;
        struct {
            pid_t pid;
            vaddr_t wstatus;
            int options;
        } wait4;
        struct {
            int status;
        } exit;
        struct {
            int code;
            vaddr_t addr;
        } arch_prctl;
        struct {
            vaddr_t buf;
        } uname;
    };
};

struct file;
struct proc {
    list_elem_t next;
    enum proc_state state;
    struct proc *parent;
    pid_t pid;
    task_t task;
    struct file *exec;
    char name[16];
    struct mm mm;
    struct file *files[FD_MAX];
    vaddr_t current_brk;
    struct mchunk *stack;

    struct mchunk *file_header;
    struct elf64_ehdr *ehdr;
    struct elf64_phdr *phdrs;

    struct syscall_context syscall;
    struct abi_emu_frame frame;

    uint64_t fsbase;
    uint64_t gsbase;
};

extern struct proc *init_proc;
extern struct waitqueue waiting_procs_wq;

struct proc *proc_alloc(void);
struct file *get_file_by_fd(struct proc *proc, fd_t fd);
pid_t proc_fork(struct proc *parent);
struct waitqueue;
void proc_block(struct proc *proc, struct waitqueue *wq);
void proc_resume(struct proc *proc);
pid_t proc_try_wait(struct proc *proc, pid_t pid, int *wstatus, int options);
void proc_exit(struct proc *proc);
errno_t proc_execve(struct proc *proc, const char *path, char *argv[],
                    char *envp[]);
struct proc *proc_lookup_by_task(task_t task);
void proc_init(void);

#endif
