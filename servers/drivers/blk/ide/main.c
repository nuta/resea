#include <resea/printf.h>
#include <resea/malloc.h>
#include <resea/ipc.h>
#include <driver/io.h>
#include <string.h>
#include "ide.h"

#define BUF_SIZE 8192
static io_t ide_io;

static void ide_init(offset_t sector) {
  io_write8(ide_io, IDE_REG_SECCNT, 1);
  io_write8(ide_io, IDE_REG_LBA_LO, sector & 0xff);
  io_write8(ide_io, IDE_REG_LBA_MID, (sector >> 8) & 0xff);
  io_write8(ide_io, IDE_REG_LBA_HI, (sector >> 16) & 0xff);
  io_write8(ide_io, IDE_REG_DRIVE,
            IDE_DRIVE_LBA | IDE_DRIVE_PRIMARY | ((sector >> 24) & 0x0f));
}

static void ide_wait(void) {
    while ((io_read8(ide_io, IDE_REG_STATUS) & IDE_STATUS_DRQ) == 0);
}

static error_t ide_read(offset_t sector, uint8_t *buf, size_t len) {
    ASSERT(IS_ALIGNED(len , sizeof(uint16_t)));
    ide_init(sector);
    io_write8(ide_io, IDE_REG_CMD, IDE_CMD_READ);
    ide_wait();

    for (size_t i = 0; i < len; i += 2) {
        uint16_t data = io_read16(ide_io, IDE_REG_DATA);
        buf[i + 1] = data >> 8;
        buf[i] = data & 0xff;
    }

    return OK;
}

static error_t ide_write(offset_t sector, const uint8_t *buf, size_t len) {
    ASSERT(IS_ALIGNED(len , sizeof(uint16_t)));
    ide_init(sector);
    io_write8(ide_io, IDE_REG_CMD, IDE_CMD_WRITE);
    ide_wait();

    for (size_t i = 0; i < len; i += 2) {
        io_write16(ide_io, IDE_REG_DATA, (buf[i + 1] << 8) | buf[i]);
    }
    return OK;
}

void main(void) {
    ide_io = io_alloc_port(0x1f0, 0x10, IO_ALLOC_NORMAL);

    ASSERT_OK(ipc_serve("disk"));
    TRACE("ready");
    while (true) {
        struct message m;
        error_t err = ipc_recv(IPC_ANY, &m);
        ASSERT_OK(err);

        // TODO: get the disk size
        switch (m.type) {
            case BLK_READ_MSG: {
                size_t sector = m.blk_read.sector;
                size_t len = m.blk_read.num_sectors * SECTOR_SIZE;
                if (len > BUF_SIZE) {
                    ipc_reply_err(m.src, ERR_NOT_ACCEPTABLE);
                    break;
                }

                uint8_t buf[BUF_SIZE];
                error_t err = ide_read(sector, buf, len);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                m.type = BLK_READ_REPLY_MSG;
                m.blk_read_reply.data = buf;
                m.blk_read_reply.data_len = len;
                ipc_reply(m.src, &m);
                break;
            }
            case BLK_WRITE_MSG: {
                error_t err = ide_write(m.blk_write.sector, m.blk_write.data, m.blk_write.data_len);
                free((void *) m.blk_write.data);
                if (err != OK) {
                    ipc_reply_err(m.src, err);
                    break;
                }

                m.type = BLK_WRITE_REPLY_MSG;
                ipc_reply(m.src, &m);
                break;
            }
            default:
                TRACE("unknown message %d", m.type);
        }
    }
}
