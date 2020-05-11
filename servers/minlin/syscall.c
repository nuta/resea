#include <resea/syscall.h>
#include <resea/malloc.h>
#include <cstring.h>
#include "elf.h"
#include "fs.h"
#include "mm.h"
#include "abi.h"
#include "proc.h"
#include "syscall.h"

static uint8_t tmpbuf[PAGE_SIZE * 2];

long sys_open(struct proc *proc) {
    vaddr_t path = proc->syscall.open.path;
    int flags = proc->syscall.open.flags;
    mode_t mode = proc->syscall.open.mode;

    // Read the path string.
    char pathbuf[PATH_MAX];
    if (strncpy_from_user(proc, pathbuf, path, PATH_MAX) == PATH_MAX) {
        return -ENAMETOOLONG;
    }

    DBG("open = '%s'", pathbuf);

    // Open the file in the filesystem driver.
    return fs_proc_open(proc, pathbuf, flags, mode);
}

int pre_open(struct proc *proc, vaddr_t path, int flags, mode_t mode) {
    TRACE("%s: sys_open(path=%p)", proc->name, path);
    proc->syscall.open.path = path;
    proc->syscall.open.flags = flags;
    proc->syscall.open.mode = mode;
    return 0;
}

long sys_close(struct proc *proc) {
    // TODO:
    return 0;
}

int pre_close(struct proc *proc, fd_t fd) {
    TRACE("%s: sys_close(fd=%d)", proc->name, fd);
    proc->syscall.close.fd = fd;
    return 0;
}

long sys_stat(struct proc *proc) {
    vaddr_t path = proc->syscall.stat.path;
    vaddr_t stat = proc->syscall.stat.stat;

    // Read the path string.
    char pathbuf[PATH_MAX];
    if (strncpy_from_user(proc, pathbuf, path, PATH_MAX) == PATH_MAX) {
        return -ENAMETOOLONG;
    }

    int err;
    struct stat statbuf;
    if ((err = fs_stat(pathbuf, &statbuf)) < 0) {
        return err;
    }

    copy_to_user(proc, stat, &statbuf, sizeof(statbuf));
    return 0;
}

int pre_stat(struct proc *proc, vaddr_t path, vaddr_t stat) {
    TRACE("%s: sys_stat(path=%p, stat=%p)", proc->name, path, stat);
    proc->syscall.stat.path = path;
    proc->syscall.stat.stat = stat;
    return 0;
}

long sys_lstat(struct proc *proc) {
    NYI();
    return 0;
}

int pre_lstat(struct proc *proc, vaddr_t path, vaddr_t stat) {
    TRACE("%s: sys_lstat(path=%p, stat=%p)", proc->name, path, stat);
    NYI();
    return 0;
}

long sys_write(struct proc *proc) {
    struct file *file = proc->syscall.write.file;
    vaddr_t buf = proc->syscall.write.buf;
    size_t len = proc->syscall.write.len;

    if (copy_from_user(proc, tmpbuf, buf, len) != OK) {
        return -EFAULT;
    }

    fs_write(file, tmpbuf, len);
    tmpbuf[len] = '\0';
    DBG("write = '%s'", tmpbuf);

    return len;
}

ssize_t pre_write(struct proc *proc, fd_t fd, vaddr_t buf, size_t len) {
    TRACE("%s: sys_write(fd=%d, buf=%p, len=%d)", proc->name, fd, buf, len);

    struct file *file = get_file_by_fd(proc, fd);
    if (!file) {
        return -EBADFD;
    }

    proc->syscall.write.file = file;
    proc->syscall.write.buf = buf;
    proc->syscall.write.len = len;
    return 0;
}

long sys_writev(struct proc *proc) {
    struct file *file = proc->syscall.writev.file;
    vaddr_t iovbase = proc->syscall.writev.iovbase;
    int iovcnt = proc->syscall.writev.iovcnt;

    size_t written_len = 0;
    for (int i = 0; i < iovcnt; i++, iovbase += sizeof(struct iovec)) {
        struct iovec iov;
        if (copy_from_user(proc, &iov, iovbase, sizeof(iov)) != OK) {
            return -EFAULT;
        }

        if (copy_from_user(proc, tmpbuf, iov.base, iov.len) != OK) {
            return -EFAULT;
        }

        fs_write(file, tmpbuf, iov.len);
        written_len += iov.len;

        if (tmpbuf[0] != '\n')  {
            tmpbuf[iov.len] = '\0';
            DBG("writev = '%s'", tmpbuf);
        }
    }

    return written_len;
}

int pre_writev(struct proc *proc, fd_t fd, vaddr_t iovbase, int iovcnt) {
    TRACE("%s: sys_writev(fd=%d, iov=%p, iovcnt=%d)",
          proc->name, fd, iovbase, iovcnt);

    struct file *file = get_file_by_fd(proc, fd);
    if (!file) {
        return -EBADFD;
    }

    proc->syscall.writev.file = file;
    proc->syscall.writev.iovbase = iovbase;
    proc->syscall.writev.iovcnt = iovcnt;
    return 0;
}

static long sys_read(struct proc *proc) {
    struct file *file = proc->syscall.read.file;
    size_t buf = proc->syscall.read.buf;
    size_t len = proc->syscall.read.len;
    ssize_t read_len = fs_read(file, tmpbuf, len /* FIXME: */);
    if (read_len < 0) {
        if (read_len == -EAGAIN) {
            proc_block(proc, &file->inode->read_wq);
            return -EBLOCKED;
        }

        return read_len;
    }

    // TODO: handle erros
    copy_to_user(proc, buf, tmpbuf, read_len);
    return read_len;
}

static int pre_read(struct proc *proc, fd_t fd, vaddr_t buf, size_t len) {
    TRACE("%s: sys_read(fd=%d)", proc->name, fd);
    struct file *file = get_file_by_fd(proc, fd);
    if (!file) {
        return -EBADFD;
    }

    proc->syscall.read.file = file;
    proc->syscall.read.buf = buf;
    proc->syscall.read.len = len;
    return 0;
}

long sys_ioctl(struct proc *proc) {
    return 0;
}

int pre_ioctl(struct proc *proc, fd_t fd, unsigned cmd, unsigned arg) {
    TRACE("%s: sys_ioctl(fd=%d, cmd=%p, arg=%p)", proc->name, fd, cmd, arg);
    // TODO:
    return 0;
}

long sys_access(struct proc *proc) {
    return -ENOENT;
}

int pre_access(struct proc *proc, vaddr_t path, int mode) {
    TRACE("%s: sys_access(path=%p, mode=%d)", proc->name, path, mode);
    return 0;
}

long sys_brk(struct proc *proc) {
    return proc->current_brk;
}

int pre_brk(struct proc *proc, vaddr_t addr) {
    TRACE("%s: sys_brk(addr=%p)", proc->name, addr);
    if (addr >= proc->current_brk) {
        if (addr >= HEAP_END) {
            return -ENOMEM;
        }

        proc->current_brk  = addr;
    } else {
        // Do nothing and return the current break. Typically, brk(NULL).
    }

    return 0;
}

long sys_mmap(struct proc *proc) {
    return 0;
}

int pre_mmap(struct proc *proc, vaddr_t addr, size_t len, int prot,
                 int flags, fd_t fd, loff_t off) {
    TRACE("%s: sys_mmap(addr=%p, len=%d, flags=%x)",
          proc->name, addr, len, flags);
    // TODO:
    NYI();
    return 0;
}

long sys_fork(struct proc *proc) {
    return proc_fork(proc);
}

pid_t pre_fork(struct proc *proc) {
    return 0;
}

long sys_execve(struct proc *proc) {
    vaddr_t path = proc->syscall.execve.path;
    vaddr_t argv = proc->syscall.execve.argv;
    vaddr_t envp = proc->syscall.execve.envp;

    char pathbuf[PATH_MAX];
    if (strncpy_from_user(proc, pathbuf, path, PATH_MAX) == PATH_MAX) {
        return -ENAMETOOLONG;
    }

    char *argvbuf[ARGV_MAX];
    int i;
    for (i = 0; i < ARGV_MAX - 1; i++) {
        vaddr_t user_ptr;
        if (copy_from_user(proc, &user_ptr, argv, sizeof(user_ptr)) != OK) {
            return -EFAULT;
        }

        if (!user_ptr) {
            break;
        }

        char *strbuf = malloc(PATH_MAX); // FIXME:
        if (strncpy_from_user(proc, strbuf, user_ptr, PATH_MAX) == PATH_MAX) {
            return -EINVAL; // FIXME: Use an appropriate errno
        }

        argvbuf[i] = strbuf;
        argv += sizeof(uintptr_t);
    }

    if (i == ARGV_MAX - 1) {
        return -EINVAL; // FIXME: Use an appropriate errno
    }
    argvbuf[i] = NULL;

    char *envpbuf[] = { NULL }; // TODO:
    errno_t err;
    if ((err = proc_execve(proc, pathbuf, argvbuf, envpbuf)) < 0) {
        return err;
    }

    // Returns EBLOCKED since the caller *task* no longer exist.
    return -EBLOCKED;
}

int pre_execve(struct proc *proc, vaddr_t path, vaddr_t argv, vaddr_t envp) {
    TRACE("%s: sys_execve(path=%p, argv=%p, envp=%p)",
          proc->name, path, argv, envp);
    proc->syscall.execve.path = path;
    proc->syscall.execve.argv = argv;
    proc->syscall.execve.envp = envp;
    return 0;
}

long sys_wait4(struct proc *proc) {
    pid_t pid = proc->syscall.wait4.pid;
    vaddr_t wstatus = proc->syscall.wait4.wstatus;
    int options = proc->syscall.wait4.options;

    int wstatusbuf;
    pid_t ret = proc_try_wait(proc, pid, &wstatusbuf, options);
    if (ret == -EAGAIN) {
        proc_block(proc, &waiting_procs_wq);
        return -EBLOCKED;
    }

    if (wstatus) {
        // TODO: copy to user
    }

    return ret;
}

int pre_wait4(struct proc *proc, pid_t pid, vaddr_t wstatus, int options,
              UNUSED vaddr_t rusage) {
    TRACE("%s: sys_wait4()", proc->name);
    proc->syscall.wait4.pid = pid;
    proc->syscall.wait4.wstatus = wstatus;
    proc->syscall.wait4.options = options;
    return 0;
}

long sys_exit(struct proc *proc) {
    proc_exit(proc);

    // Returns EBLOCKED since the caller process no longer exist.
    return -EBLOCKED;
}

int pre_exit(struct proc *proc, int status) {
    TRACE("%s: sys_exit(status=%d)", proc->name, status);
    proc->syscall.exit.status = status;
    return 0;
}

long sys_getpid(struct proc *proc) {
    return proc->pid;
}

int pre_getpid(struct proc *proc) {
    TRACE("%s: sys_getpid()", proc->name);
    return 0;
}

long sys_rt_sigaction(struct proc *proc) {
    return 0;
}

int pre_rt_sigaction(struct proc *proc, int signum, vaddr_t act, vaddr_t oldact,
                         size_t sigsetsize) {
    TRACE("%s: sys_rt_sigaction(argnum=%d, act=%p, oldact=%p, sigsetsize=%d)",
          proc->name, act, oldact, sigsetsize);
    // TODO:
    return 0;
}

long sys_rt_sigprocmask(struct proc *proc) {
    return 0;
}

int pre_rt_sigprocmask(struct proc *proc, int how, vaddr_t set,
                       vaddr_t oldset) {
    TRACE("%s: sys_rt_sigprocmask(how=%d, set=%p, oldset=%p)",
          proc->name, how, set, oldset);
    // TODO:
    return 0;
}

long sys_arch_prctl(struct proc *proc) {
    int code = proc->syscall.arch_prctl.code;
    vaddr_t addr = proc->syscall.arch_prctl.addr;
    TRACE("%s: sys_arch_prctl(code=%x, addr=%p)", proc->name, code, addr);
    switch (code) {
        case ARCH_SET_FS:
            proc->fsbase = addr;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

int pre_arch_prctl(struct proc *proc, int code, vaddr_t addr) {
    proc->syscall.arch_prctl.code = code;
    proc->syscall.arch_prctl.addr = addr;
    return 0;
}

long sys_set_tid_address(struct proc *proc) {
    // TODO:
    return 0;
}

int pre_set_tid_address(struct proc *proc, vaddr_t tidptr) {
    TRACE("%s: sys_tid_address(tidptr=%p)", proc->name, tidptr);
    return 0;
}

static struct utsname uname = {
    .sysname = "Resea Linux ABI Emulation Layer",
    .nodename = "",
    .release = "",
    .version = __VERSION__,
    .machine = "",
};

long sys_uname(struct proc *proc) {
    vaddr_t buf = proc->syscall.uname.buf;
    if (copy_to_user(proc, buf, &uname, sizeof(uname)) != OK) {
        return -EFAULT;
    }

    return 0;
}

ssize_t pre_uname(struct proc *proc, vaddr_t buf) {
    TRACE("%s: sys_uname(buf=%p)", proc->name, buf);
    proc->syscall.uname.buf = buf;
    return 0;
}

static void dispatch(struct proc *proc, int syscall, uintmax_t arg0,
                     uintmax_t arg1, uintmax_t arg2, uintmax_t arg3,
                     uintmax_t arg4, uintmax_t arg5) {
    int err;
    long (*handler)(struct proc *proc) = NULL;
    switch (syscall) {
        case SYS_EXIT:
            err = pre_exit(proc, arg0);
            handler = sys_exit;
            break;
        case SYS_OPEN:
            err = pre_open(proc, arg0, arg1, arg2);
            handler = sys_open;
            break;
        case SYS_CLOSE:
            err = pre_close(proc, arg0);
            handler = sys_close;
            break;
        case SYS_STAT:
            err = pre_stat(proc, arg0, arg1);
            handler = sys_stat;
            break;
        case SYS_LSTAT:
            err = pre_lstat(proc, arg0, arg1);
            handler = sys_lstat;
            break;
        case SYS_READ:
            err = pre_read(proc, arg0, arg1, arg2);
            handler = sys_read;
            break;
        case SYS_WRITE:
            err = pre_write(proc, arg0, arg1, arg2);
            handler = sys_write;
            break;
        case SYS_WRITEV:
            err = pre_writev(proc, arg0, arg1, arg2);
            handler = sys_writev;
            break;
        case SYS_IOCTL:
            err = pre_ioctl(proc, arg0, arg1, arg2);
            handler = sys_ioctl;
            break;
        case SYS_ACCESS:
            err = pre_access(proc, arg0, arg1);
            handler = sys_access;
            break;
        case SYS_BRK:
            err = pre_brk(proc, arg0);
            handler = sys_brk;
            break;
        case SYS_MMAP:
            err = pre_mmap(proc, arg0, arg1, arg2, arg3, arg4, arg5);
            handler = sys_mmap;
            break;
        case SYS_FORK:
            err = pre_fork(proc);
            handler = sys_fork;
            break;
        case SYS_EXECVE:
            err = pre_execve(proc, arg0, arg1, arg2);
            handler = sys_execve;
            break;
        case SYS_WAIT4:
            err = pre_wait4(proc, arg0, arg1, arg2, arg3);
            handler = sys_wait4;
            break;
        case SYS_GETPID:
            err = pre_getpid(proc);
            handler = sys_getpid;
            break;
        case SYS_RT_SIGACTION:
            err = pre_rt_sigaction(proc, arg0, arg1, arg2, arg3);
            handler = sys_rt_sigaction;
            break;
        case SYS_RT_SIGPROCMASK:
            err = pre_rt_sigprocmask(proc, arg0, arg1, arg2);
            handler = sys_rt_sigprocmask;
            break;
        case SYS_ARCH_PRCTL:
            err = pre_arch_prctl(proc, arg0, arg1);
            handler = sys_arch_prctl;
            break;
        case SYS_SET_TID_ADDRESS:
            err = pre_set_tid_address(proc, arg0);
            handler = sys_set_tid_address;
            break;
        case SYS_UNAME:
            err = pre_uname(proc, arg0);
            handler = sys_uname;
            break;
        default:
            err = -ENOSYS;
            WARN("unknown syscall %d (args=[%p, %p, ...])",
                 syscall, arg0, arg1);
    }

    proc->syscall.err = err;
    proc->syscall.handler = handler;
    try_syscall(proc);
}

void try_syscall(struct proc *proc) {
    long ret;
    if (proc->syscall.err < 0) {
        ret = proc->syscall.err;
    } else {
        ret = proc->syscall.handler(proc);
    }

    // Resume the caller process if the system call handing has finished. It is
    // guaranteed that in Linux, no system calls returns -4095 <= x <= -1 as a
    // valid result, according to a comment in glibc.
    if ((ret >= 0 || ret < -4095) || ret != -EBLOCKED) {
        // FIXME: arch-specific
        proc->frame.rax = ret;
        proc->frame.fsbase = proc->fsbase;
        proc->frame.gsbase = proc->gsbase;

        struct message m;
        m.type = ABI_HOOK_REPLY_MSG;
        memcpy(&m.abi_hook_reply.frame, &proc->frame,
               sizeof(m.abi_hook_reply.frame));
        ipc_reply(proc->task, &m);
    }
}

void handle_syscall(struct proc *proc, struct abi_emu_frame *args) {
    memcpy(&proc->frame, args, sizeof(proc->frame));
    dispatch(proc, args->rax, args->rdi, args->rsi, args->rdx,
             args->r10, args->r8, args->r9);
}
