#ifndef __BOOTFS_H__
#define __BOOTFS_H__

#include <types.h>

struct bootfs_header {
    uint32_t version;
    uint32_t files_off;
    uint32_t num_files;
    uint32_t padding;
} __packed;

struct bootfs_file {
    char name[48];
    uint32_t offset;
    uint32_t len;
    uint8_t padding[8];
} __packed;

#endif
