#ifndef __ABI_H__
#define __ABI_H__

#include <types.h>

#define ARGV_MAX 32
#define PATH_MAX 256

typedef int fd_t;
typedef int pid_t;
typedef unsigned mode_t;

/// A length or an error value (if it's negative).
typedef intmax_t ssize_t;

/// A negated error number.
typedef int errno_t;

/// A offset type.
typedef int loff_t;

//
//  errno
//

/// No such file or directory.
#define ENOENT 2
/// Invalid exec format.
#define ENOEXEC 8
/// Try again: too many processes, etc.
#define EAGAIN 11
/// Run out of memory.
#define ENOMEM 12
/// Invalid user pointer.
#define EFAULT 14
/// Invalid argument.
#define EINVAL 22
/// Too many opened files.
#define EMFILE 24
/// FIXME: Indicates that something went wrong but we've not converted the error
/// into an appropriate errno.
#define EDOM 33
/// Too long file path.
#define ENAMETOOLONG 36
/// Invalid system call number.
#define ENOSYS 38
/// Invalid file descriptor.
#define EBADFD 77

/// FIXME: An internally used error value indicating that the current system
/// call needs to be blocked. We don't use EXDEV.
#define EBLOCKED 18

// lseek(2) whence values.
#define SEEK_SET 0
#define SEEK_CUR 1

// arch_prctl(2) subfunctions
#define ARCH_SET_FS 0x1002

// iov
struct iovec {
    vaddr_t base;
    size_t len;
};

/// Used by sys_rt_sigaction(2).
struct sigaction {
    // TODO:
};

/// The stat struct.
struct stat {
    unsigned long st_dev;
    unsigned long st_ino;
    unsigned long st_nlink;
    unsigned st_mode;
    unsigned st_uid;
    unsigned st_gid;
    unsigned __pad0;
    unsigned long st_rdev;
    long st_size;
    // TODO: Add remaining fields.
} __packed;

/// Used by uname(2).
struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
};

/// Used by sys_rt_sigprocmask(2).
typedef unsigned long old_sigset_t;

//
//  System call numbers
//  See
//  https://elixir.bootlin.com/linux/latest/source/arch/x86/entry/syscalls/syscall_64.tbl
//
#define SYS_READ            0
#define SYS_WRITE           1
#define SYS_OPEN            2
#define SYS_CLOSE           3
#define SYS_STAT            4
#define SYS_LSTAT           6
#define SYS_MMAP            9
#define SYS_MPROTECT        10
#define SYS_BRK             12
#define SYS_RT_SIGACTION    13
#define SYS_RT_SIGPROCMASK  14
#define SYS_IOCTL           16
#define SYS_WRITEV          20
#define SYS_ACCESS          21
#define SYS_GETPID          39
#define SYS_FORK            57
#define SYS_EXECVE          59
#define SYS_EXIT            60
#define SYS_WAIT4           61
#define SYS_UNAME           63
#define SYS_ARCH_PRCTL      158
#define SYS_SET_TID_ADDRESS 218

//
// Auxiliary vector types
//

/// End of a vector.
#define AT_NULL 0
/// The address of the ELF program headers.
#define AT_PHDR 3
/// The size of a program header.
#define AT_PHENT 4
/// The number of program heaers.
#define AT_PHNUM 5
/// The size of a page.
#define AT_PAGESZ 6
/// The address of random 16 bytes (used for stack canary set by libc).
#define AT_RANDOM 25
//
//  minlin-specific constants
//
#define HEAP_ADDR       0xa0000000
#define HEAP_END        0xe0000000
#define STACK_ADDR      0xe0000000
#define STACK_TOP       0x0000a000
#define STACK_STRBUF    0x0000c000
#define STACK_LEN       0x00010000
#define ELF_HEADER_ADDR 0xf0000000
#define ELF_HEADER_LEN  0x00001000

#endif
