#ifndef __INITFS_H__
#define __INITFS_H__

#include <types.h>

struct initfs_header {
    uint8_t startup_code[16];
    uint32_t version;
    uint32_t files_off;
    uint32_t num_files;
    uint32_t padding;
} PACKED;

struct initfs_file {
    char name[48];
    uint32_t offset;
    uint32_t len;
    uint8_t padding[8];
} PACKED;

#endif
