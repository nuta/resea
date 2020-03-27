#include <std/printf.h>
#include <std/syscall.h>
#include <std/lookup.h>
#include <std/malloc.h>
#include <std/map.h>
#include <std/string.h>
#include <std/rand.h>
#include <message.h>
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

    struct fat_dir dir;
    struct fat_dirent *e;
    ASSERT_OK(fat_opendir(&fs, &dir, "/apps"));
    while ((e = fat_readdir(&fs, &dir)) != NULL) {
        DBG("/APPS/%s", e->name);
    }

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

                handle_t handle;
                rand_bytes((uint8_t *) &handle, sizeof(handle));
                map_set_handle(clients, &handle, file);

                m.type = FS_OPEN_REPLY_MSG;
                m.fs_open_reply.handle = handle;
                ipc_reply(m.src, &m);
                break;
            }
            case FS_READ_MSG: {
                struct fat_file *file =
                    map_get_handle(clients, &m.fs_read.handle);
                if (!file) {
                    ipc_reply_err(m.src, ERR_NOT_FOUND);
                    break;
                }

                size_t max_len = MIN(8192, m.fs_read.len);
                void *buf = malloc(max_len);
                error_t err =
                    fat_read(&fs, file, m.fs_read.offset, buf, m.fs_read.len);
                if (IS_ERROR(err)) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                m.type = FS_READ_REPLY_MSG;
                m.fs_read_reply.data = buf;
                m.fs_read_reply.len = m.fs_read.len;
                ipc_reply(m.src, &m);
                free(buf);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
