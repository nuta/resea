#include "fat.h"
#include <resea/handle.h>
#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

static task_t ramdisk_server;

void blk_read(size_t sector, void *buf, size_t num_sectors) {
    struct message m;
    m.type = BLK_READ_MSG;
    m.blk_read.sector = sector;
    m.blk_read.num_sectors = num_sectors;
    error_t err = ipc_call(ramdisk_server, &m);
    ASSERT(IS_OK(err));
    ASSERT(m.type == BLK_READ_REPLY_MSG);
    memcpy(buf, m.blk_read_reply.data, m.blk_read_reply.data_len);
}

void blk_write(size_t sector, const void *buf, size_t num_sectors) {
    struct message m;
    m.type = BLK_WRITE_MSG;
    m.blk_write.sector = sector;
    m.blk_write.data = (void *) buf;
    m.blk_write.data_len = num_sectors * SECTOR_SIZE;
    error_t err = ipc_call(ramdisk_server, &m);
    ASSERT(IS_OK(err));
    ASSERT(m.type == BLK_WRITE_REPLY_MSG);
}

void main(void) {
    TRACE("starting...");

    ramdisk_server = ipc_lookup("disk");
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
        strncpy2(tmp, (const char *) e->name, sizeof(tmp));
        DBG("/%s", tmp);
    }
    DBG("---------------------------------------------------");

    ASSERT_OK(ipc_serve("fs"));

    TRACE("ready");
    while (true) {
        struct message m;
        bzero(&m, sizeof(m));
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case FS_OPEN_MSG: {
                struct fat_file *file = malloc(sizeof(*file));
                error_t err = fat_open(&fs, file, m.fs_open.path);
                if (IS_ERROR(err)) {
                    free(file);
                    ipc_reply_err(m.src, err);
                    break;
                }

                handle_t handle = handle_alloc(m.src);
                handle_set(m.src, handle, file);

                m.type = FS_OPEN_REPLY_MSG;
                m.fs_open_reply.handle = handle;
                ipc_reply(m.src, &m);
                break;
            }
            case FS_CREATE_MSG: {
                struct fat_file *file = malloc(sizeof(*file));
                error_t err = fat_create(&fs, file, m.fs_create.path,
                                         m.fs_create.exist_ok);
                if (IS_ERROR(err)) {
                    free(file);
                    ipc_reply_err(m.src, err);
                    break;
                }

                handle_t handle = handle_alloc(m.src);
                handle_set(m.src, handle, file);

                m.type = FS_CREATE_REPLY_MSG;
                m.fs_create_reply.handle = handle;
                ipc_reply(m.src, &m);
                break;
            }
            case FS_READ_MSG: {
                struct fat_file *file = handle_get(m.src, m.fs_read.handle);
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
                m.fs_read_reply.data_len = len_or_err;
                ipc_reply(m.src, &m);
                free(buf);
                break;
            }
            case FS_WRITE_MSG: {
                struct fat_file *file = handle_get(m.src, m.fs_write.handle);
                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                error_t err = fat_write(&fs, file, m.fs_write.offset,
                                        m.fs_write.data, m.fs_write.data_len);
                if (IS_ERROR(err)) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                m.type = FS_WRITE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
