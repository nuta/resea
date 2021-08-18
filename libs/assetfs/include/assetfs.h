#ifndef __ASSETFS_H__
#define __ASSETFS_H__

#include <types.h>

#define ASSETFS_MAGIC 0xaa210001

struct __packed assetfs_file {
    char name[48];
    uint32_t offset;
    uint32_t size;
    uint8_t padding[8];
};

struct __packed assetfs_header {
    uint32_t magic;
    uint32_t num_entries;
    uint8_t padding[8];
    struct assetfs_file entries[];
};

struct assetfs {
    uint8_t *image;
    uint32_t num_entries;
    struct assetfs_file *entries;
};

error_t assetfs_open(struct assetfs *fs, void *image, size_t image_size);
struct assetfs_file *assetfs_open_file(struct assetfs *fs, const char *name);
void *assetfs_file_data(struct assetfs *fs, struct assetfs_file *file);

#endif
