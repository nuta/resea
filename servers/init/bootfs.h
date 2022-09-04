#pragma once
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

struct bootfs_file *bootfs_open(unsigned index);
void bootfs_read(struct bootfs_file *file, offset_t off, void *buf, size_t len);
void bootfs_init(void);
