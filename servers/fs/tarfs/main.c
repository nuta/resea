#include <resea/printf.h>
#include <resea/malloc.h>
#include <resea/handle.h>
#include <resea/ipc.h>
#include <string.h>
#include <list.h>

extern char __tarball[];
extern char __tarball_end[];
static list_t files;

#define TAR_TYPE_NORMAL   '0'
#define TAR_TYPE_SYMLINK  '2'
#define TAR_TYPE_DIR      '5'

struct tar_header {
    char filename[100];
    char mode[8];
    char owner_id[8];
    char group_id[8];
    char size[12];
    char time_modified[12];
    char checksum[8];
    char type;
    char linked_to[100];
    char ustar[6];
    char ustar_version[2];
    char owner_name[32];
    char group_name[32];
    char device_major[8];
    char device_minor[8];
    char filename_prefix[155];
    char padding[12];
    char data[];
} __packed;

struct file {
    list_elem_t next;
    const char *path;
    uint8_t *data;
    size_t len;
    char type;
    const char *linked_to;
};

static int str2int(const char *s) {
    unsigned long x = 0;
    while (*s) {
        x = (x * 8) + (*s - '0');
        s++;
    }
    return x;
}

static void load_all_files(void) {
    struct tar_header *header = (struct tar_header *) __tarball;
    while ((uintptr_t) header < (uintptr_t) __tarball_end) {
        if (header->filename[0] == '\0') {
            break;
        }

        struct file *file = malloc(sizeof(*file));
        if (header->filename[0] == '/') {
            file->path = &header->filename[1];
        } else {
            file->path = header->filename;
        }

        file->type = header->type;
        file->len = str2int(header->size);
        file->data = malloc(file->len);
        file->linked_to =
            (file->type == TAR_TYPE_SYMLINK) ? header->linked_to : NULL;
        memcpy(file->data, header->data, file->len);
        list_push_back(&files, &file->next);
        header = (struct tar_header *)
            ALIGN_UP((uintptr_t) header + sizeof(*header) + file->len, 512);
        DBG("tarfs: %s (len=%d, type=%c) %d %p",
            file->path, file->len, file->type);
    }
}

static struct file *open(const char *path) {
    if (path[0] == '/') {
        path = &path[1];
    }

    LIST_FOR_EACH(file, &files, struct file, next) {
        if (!strcmp(file->path, path)) {
            if (file->linked_to) {
                return open(file->linked_to);
            }

            return file;
        }
    }

    return NULL;
}

static int read(struct file *file, offset_t off, void *buf, size_t len) {
    if (off >= file->len) {
        return 0;
    }

    size_t read_len = MIN(len, file->len - off);
    memcpy(buf, &file->data[off], read_len);
    return read_len;
}

void main(void) {
    TRACE("starting...");
    list_init(&files);
    load_all_files();
    ASSERT_OK(ipc_serve("fs"));

    TRACE("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case FS_OPEN_MSG: {
                struct file *file = open(m.fs_open.path);
                free(m.fs_open.path);
                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                handle_t handle = handle_alloc(m.src);
                handle_set(m.src, handle, file);
                m.type = FS_OPEN_REPLY_MSG;
                m.fs_open_reply.handle = handle;
                ipc_reply(m.src, &m);
                break;
            }
            case FS_STAT_MSG: {
                struct file *file = open(m.fs_stat.path);
                free(m.fs_stat.path);
                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                m.type = FS_STAT_REPLY_MSG;
                m.fs_stat_reply.size = file->len;
                ipc_reply(m.src, &m);
                break;
            }
            case FS_READ_MSG: {
                struct file *file = handle_get(m.src, m.fs_read.handle);
                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                size_t max_len = MIN(8192, m.fs_read.len);
                void *buf = malloc(max_len);
                int read_len = read(file, m.fs_read.offset, buf, m.fs_read.len);
                m.type = FS_READ_REPLY_MSG;
                m.fs_read_reply.data = buf;
                m.fs_read_reply.data_len = read_len;
                ipc_reply(m.src, &m);
                free(buf);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
