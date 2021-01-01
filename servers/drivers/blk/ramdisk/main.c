#include <resea/ipc.h>
#include <resea/malloc.h>
#include <resea/printf.h>
#include <string.h>

#define SECTOR_SIZE 512
#define BUF_SIZE    8192
extern uint8_t __image[];
extern uint8_t __image_end[];

void main(void) {
    ASSERT_OK(ipc_serve("disk"));
    TRACE("ready");
    size_t disk_size = (uintptr_t) __image_end - (uintptr_t) __image;
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        switch (m.type) {
            case BLK_READ_MSG: {
                size_t offset = m.blk_read.sector * SECTOR_SIZE;
                size_t len = m.blk_read.num_sectors * SECTOR_SIZE;
                if (offset + len > disk_size || offset + len < offset) {
                    ipc_reply_err(m.src, ERR_NOT_ACCEPTABLE);
                    break;
                }

                m.type = BLK_READ_REPLY_MSG;
                m.blk_read_reply.data = &__image[offset];
                m.blk_read_reply.data_len = len;
                ipc_reply(m.src, &m);
                break;
            }
            case BLK_WRITE_MSG: {
                size_t offset = m.blk_write.sector * SECTOR_SIZE;
                size_t len = m.blk_write.data_len;
                if (offset + len > disk_size || offset + len < offset
                    || !IS_ALIGNED(len, SECTOR_SIZE)) {
                    ipc_reply_err(m.src, ERR_NOT_ACCEPTABLE);
                    break;
                }

                memcpy(&__image[offset], m.blk_write.data, len);
                free(m.blk_write.data);

                m.type = BLK_WRITE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
