#include <resea/ipc.h>
#include <resea/printf.h>
#include <resea/malloc.h>
#include <string.h>

void fs_read(const char *path) {
    task_t fs_server = ipc_lookup("fs");
    struct message m;
    m.type = FS_OPEN_MSG;
    m.fs_open.path = (char *) path;
    error_t err = ipc_call(fs_server, &m);
    if (IS_ERROR(err)) {
        WARN("failed to open a file: '%s' (%s)", path, err2str(err));
        return;
    }

    ASSERT(m.type == FS_OPEN_REPLY_MSG);
    handle_t handle = m.fs_open_reply.handle;

    size_t offset = 0;
    while (true) {
        m.type = FS_READ_MSG;
        m.fs_read.handle = handle;
        m.fs_read.offset = offset;
        m.fs_read.len = PAGE_SIZE;
        err = ipc_call(fs_server, &m);
        if (IS_ERROR(err)) {
            WARN("failed to read a file: %s", err2str(err));
            return;
        }

        ASSERT(m.type == FS_READ_REPLY_MSG);
        if (!m.fs_read_reply.data_len) {
            break;
        }

        DBG("%s", m.fs_read_reply.data);
        free((void *) m.fs_read_reply.data);
        offset += PAGE_SIZE;
    }
}
