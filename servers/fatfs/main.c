#include <resea/printf.h>
#include <resea/malloc.h>
#include <resea/map.h>
#include <resea/handle.h>
#include <resea/ipc.h>
#include <cstring.h>
#include "fat.h"

static task_t ramdisk_server;
static map_t clients;

void blk_read(size_t sector, void *buf, size_t num_sectors) {
    struct message m;
    m.type = BLK_READ_MSG;
    m.blk_read.sector = sector;
    m.blk_read.num_sectors = num_sectors;
    error_t err = ipc_call(ramdisk_server, &m);
    ASSERT(IS_OK(err));
    ASSERT(m.type == BLK_READ_REPLY_MSG);
    memcpy(buf, m.blk_read_reply.data, m.blk_read_reply.len);
}

void blk_write(size_t offset, const void *buf, size_t len) {
    PANIC("NYI");
}

void main(void) {
    TRACE("starting...");
    clients = map_new();

    ramdisk_server = ipc_lookup("ramdisk");
    ASSERT_OK(ramdisk_server);

    struct fat fs;
    if (IS_ERROR(fat_probe(&fs, blk_read, blk_write))) {
        PANIC("failed to locate a FAT file system");
    }

    DBG("Files ---------------------------------------------");
    struct fat_dir dir;
    struct fat_dirent *e;
    char tmp[12];
    ASSERT_OK(fat_opendir(&fs, &dir, "/"));
    while ((e = fat_readdir(&fs, &dir)) != NULL) {
        strncpy(tmp, (const char *) e->name, sizeof(tmp));
        DBG("/%s", tmp);
    }
    DBG("---------------------------------------------------");

    TRACE("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case FS_OPEN_MSG: {
                struct fat_file *file = malloc(sizeof(*file));
                // TODO: Ensure path is null-terminated.

                error_t err = fat_open(&fs, file, m.fs_open.path);
                if (IS_ERROR(err)) {
                    free(file);
                    ipc_reply_err(m.src, err);
                    break;
                }

                handle_t handle = handle_alloc();
                ASSERT_OK(handle);
                handle_set(handle, file);

                m.type = FS_OPEN_REPLY_MSG;
                m.fs_open_reply.handle = handle;
                ipc_reply(m.src, &m);
                break;
            }
            case FS_READ_MSG: {
                struct fat_file *file = handle_get(m.fs_read.handle);
                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                size_t max_len = MIN(8192, m.fs_read.len);
                void *buf = malloc(max_len);
                int len_or_err =
                    fat_read(&fs, file, m.fs_read.offset, buf, max_len);
                if (IS_ERROR(len_or_err)) {
                    ipc_reply_err(m.src, len_or_err);
                    break;
                }

                m.type = FS_READ_REPLY_MSG;
                m.fs_read_reply.data = buf;
                m.fs_read_reply.len = len_or_err;
                ipc_reply(m.src, &m);
                free(buf);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
