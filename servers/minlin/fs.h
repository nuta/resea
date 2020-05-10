#ifndef __FS_H__
#define __FS_H__

#include <types.h>
#include <message.h>
#include "abi.h"
#include "waitqueue.h"

struct file;
struct fs_ops {
    int (*open)(struct file *file, const char *path, int flags, mode_t mode);
    int (*stat)(const char *path, struct stat *stat);
};

struct mountpoint {
    struct fs_ops *ops;
};

/// Inode.
struct inode {
    struct waitqueue read_wq;
    union {
        struct {
            handle_t handle;
        };
    };
};

/// An opened file.
struct file_ops;
struct file {
    struct file_ops *ops;
    struct inode *inode;
    loff_t pos;
};

struct file_ops {
    int (*acquire)(struct file *file);
    int (*release)(struct file *file);
    ssize_t (*read)(struct file *file, uint8_t *buf, size_t len);
    ssize_t (*write)(struct file *file, const uint8_t *buf, size_t len);
    ssize_t (*ioctl)(struct file *file, unsigned cmd, unsigned arg);
    loff_t (*seek)(struct file *file, loff_t off, int whence);
};

struct proc;
fd_t fs_proc_open(struct proc *proc, const char *path, int flags, mode_t mode);
int fs_open(struct file **file, const char *path, int flags, mode_t mode);
int fs_stat(const char *path, struct stat *stat);
loff_t fs_tell(struct file *file);
loff_t fs_seek(struct file *file, loff_t off, int whence);
ssize_t fs_read_pos(struct file *file, void *buf, loff_t off, size_t len);
ssize_t fs_write_pos(struct file *file, const void *buf, loff_t off, size_t len);
ssize_t fs_read(struct file *file, void *buf, size_t len);
ssize_t fs_write(struct file *file, const void *buf, size_t len);
int fs_fork(struct proc *parent, struct proc *child);
void fs_init(void);

#endif
