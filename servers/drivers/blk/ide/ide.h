#ifndef __IDE_H__
#define __IDE_H__

#define SECTOR_SIZE 512

#define IDE_REG_DATA    0
#define IDE_REG_SECCNT  2
#define IDE_REG_LBA_LO  3
#define IDE_REG_LBA_MID 4
#define IDE_REG_LBA_HI  5
#define IDE_REG_DRIVE   6
#define IDE_REG_STATUS  7
#define IDE_REG_CMD     7

#define IDE_CMD_READ   0x20
#define IDE_CMD_WRITE  0x30
#define IDE_STATUS_DRQ (1 << 3)

#define IDE_DRIVE_LBA     0xe0
#define IDE_DRIVE_PRIMARY (0 << 4)

#endif
