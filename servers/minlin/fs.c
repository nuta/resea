#include "fs.h"
#include "proc.h"
#include "abi.h"
#include <cstring.h>
#include <message.h>
#include <resea/malloc.h>
#include <resea/lookup.h>
#include <resea/syscall.h>

static task_t fs_server;

extern struct file_ops dummy_file_ops;
extern struct file_ops driver_file_ops;

struct inode *inode_alloc(void) {
    struct inode *inode = malloc(sizeof(*inode));
    waitqueue_init(&inode->read_wq);
    return inode;
}

int nyi_acquire(UNUSED struct file *file) {
    NYI();
    return 0;
}

int nyi_release(UNUSED struct file *file) {
    NYI();
    return 0;
}

ssize_t nyi_read(UNUSED struct file *file, UNUSED uint8_t *buf, UNUSED size_t len) {
    NYI();
    return 0;
}

ssize_t nyi_write(UNUSED struct file *file, UNUSED const uint8_t *buf, UNUSED size_t len) {
    NYI();
    return 0;
}

ssize_t nyi_ioctl(UNUSED struct file *file, UNUSED unsigned cmd, UNUSED unsigned arg) {
    NYI();
    return 0;
}

loff_t nyi_seek(UNUSED struct file *file, UNUSED loff_t off, UNUSED int whence) {
    switch (whence) {
        case SEEK_SET:
            file->pos = off;
            break;
        case SEEK_CUR:
            file->pos += off;
            break;
        default:
            PANIC("unknown whence: %d", whence);
    }

    return file->pos;
}

int nyi_stat(UNUSED const char *path, UNUSED struct stat *stat) {
    return -ENOENT;
}

static errno_t driver_open(struct file *file, const char *path, int flags,
                           mode_t mode) {
    struct message m;
    m.type = FS_OPEN_MSG;
    m.fs_open.path = (char *) path;
    m.fs_open.len = strlen(path) + 1;
    error_t err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        return -EDOM; // FIXME: Convert error_t to errno_t.
    }

    ASSERT(m.type == FS_OPEN_REPLY_MSG);
    file->ops = &driver_file_ops;
    file->inode = inode_alloc();
    file->inode->handle = m.fs_open_reply.handle;
    return 0;
}

static ssize_t driver_read(struct file *file, uint8_t *buf, size_t len) {
    struct message m;
    m.type = FS_READ_MSG;
    m.fs_read.handle = file->inode->handle;
    m.fs_read.offset = file->pos;
    m.fs_read.len = len;
    error_t err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        return -EDOM; // FIXME: Convert error_t to errno_t.
    }

    ASSERT(m.type == FS_READ_REPLY_MSG);
    memcpy(buf, m.fs_read_reply.data, m.fs_read_reply.len);
    free(m.fs_read_reply.data);
    file->pos += m.fs_read_reply.len;
    return m.fs_read_reply.len;
}

int driver_stat(const char *path, struct stat *stat) {
    struct message m;
    m.type = FS_STAT_MSG;
    m.fs_stat.path = (char *) path;
    m.fs_stat.len = strlen(path) + 1;
    error_t err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        return -ENOENT; // FIXME: Convert error_t to errno_t.
    }

    ASSERT(m.type == FS_STAT_REPLY_MSG);
    memset(stat, 0, sizeof(*stat));
    stat->st_mode = 0100777;
    stat->st_size = m.fs_stat_reply.size;
    return 0;
}

struct fs_ops driver_ops = {
    .open = driver_open,
    .stat = driver_stat,
};

struct file_ops driver_file_ops = {
    .acquire = nyi_acquire,
    .release = nyi_release,
    .read = driver_read,
    .write = nyi_write,
    .ioctl = nyi_ioctl,
    .seek = nyi_seek,
};

struct mountpoint root_mountpoint = {
    .ops = &driver_ops,
};

extern struct file_ops tty_file_ops;
errno_t devfs_open(struct file *file, const char *path, int flags, mode_t mode) {
    static struct inode *tty_inode = NULL;
    if (!tty_inode) {
        tty_inode = inode_alloc();
    }

    file->ops = &tty_file_ops;
    file->inode = tty_inode;

    file->ops->acquire(file);
    return 0;
}

struct fs_ops devfs_ops = {
    .open = devfs_open,
    .stat = nyi_stat,
};

struct mountpoint devfs_mountpoint = {
    .ops = &devfs_ops,
};

fd_t fs_proc_open(struct proc *proc, const char *path, int flags, mode_t mode) {
    // Allocate a fd.
    fd_t fd = 0;
    while (proc->files[fd]) {
        fd++;
    }

    if (fd == FD_MAX) {
        return -EMFILE;
    }

    int err;
    struct file *file;
    if ((err = fs_open(&file, path, flags, mode)) < 0) {
        return err;
    }

    proc->files[fd] = file;
    return fd;
}

struct mountpoint *look_for_mountpoint(const char **path) {
    struct mountpoint *mount = &root_mountpoint;

    // FIXME:
    if (!strcmp(*path, "/dev/null")) {
        mount = &devfs_mountpoint;
    }

    // FIXME:
    if (!strcmp(*path, "/dev/console")) {
        mount = &devfs_mountpoint;
    }

    return mount;
}

int fs_open(struct file **file, const char *path, int flags, mode_t mode) {
    struct mountpoint *mount = look_for_mountpoint(&path);
    if (!mount) {
        return -ENOENT;
    }

    *file = malloc(sizeof(*file));
    (*file)->pos = 0;
    (*file)->inode = NULL; // To be filled by the driver.

    // Open the file in the filesystem driver.
    int err;
    if ((err = mount->ops->open(*file, path, flags, mode)) < 0) {
        return err;
    }

    // The file system driver must fill the `inode` field.
    DEBUG_ASSERT((*file)->inode);
    return 0;
}

errno_t fs_stat(const char *path, struct stat *stat) {
    struct mountpoint *mount = look_for_mountpoint(&path);
    if (!mount) {
        return -ENOENT;
    }

    return mount->ops->stat(path, stat);
}

errno_t fs_fork(struct proc *parent, struct proc *child) {
    if (parent) {
        // TODO:
    } else {
        for (fd_t fd = 0; fd < FD_MAX; fd++) {
            child->files[fd] = NULL;
        }
    }

    for (fd_t fd = 0; fd < FD_MAX; fd++) {
        child->files[fd] = NULL;
    }
    for (fd_t fd = 0; fd <= 2; fd++) {
        ASSERT(fd == fs_proc_open(child, "/dev/console", 0, 0));
    }
    return 0;
}

loff_t fs_tell(struct file *file) {
    return file->ops->seek(file, 0, SEEK_CUR);
}

loff_t fs_seek(struct file *file, loff_t off, int whence) {
    return file->ops->seek(file, off, whence);
}

ssize_t fs_read_pos(struct file *file, void *buf, loff_t off, size_t len) {
    // Save the current position.
    loff_t pos;
    if ((pos = fs_tell(file)) < 0) {
        return pos;
    }

    // Move the position.
    int err;
    if ((err = file->ops->seek(file, off, SEEK_SET)) < 0) {
        return err;
    }

    // Read the contents.
    ssize_t read_len = file->ops->read(file, buf, len);
    if (read_len < 0) {
        return read_len;
    }

    // Restore the position.
    if ((err = fs_seek(file, off ,SEEK_SET)) < 0) {
        return err;
    }

    return read_len;
}

ssize_t fs_write_pos(struct file *file, const void *buf, loff_t off, size_t len) {
    // Save the current position.
    loff_t pos;
    if ((pos = fs_tell(file)) < 0) {
        return pos;
    }

    // Move the position.
    int err;
    if ((err = file->ops->seek(file, off, SEEK_SET)) < 0) {
        return err;
    }

    // Read the contents.
    ssize_t written_len = file->ops->write(file, buf, len);
    if (written_len < 0) {
        return written_len;
    }

    // Restore the position.
    if ((err = fs_seek(file, off ,SEEK_SET)) < 0) {
        return err;
    }

    return written_len;
}

ssize_t fs_read(struct file *file, void *buf, size_t len) {
    ssize_t read_len = file->ops->read(file, buf, len);
    if (read_len < 0) {
        return read_len;
    }

    return read_len;
}

ssize_t fs_write(struct file *file, const void *buf, size_t len) {
    ssize_t written_len = file->ops->write(file, buf, len);
    if (written_len < 0) {
        return written_len;
    }

    return written_len;
}

void fs_init(void) {
    fs_server = ipc_lookup("tarfs");
    ASSERT_OK(fs_server);
}
