#ifndef __INITFS_H__
#define __INITFS_H__

#include <resea.h>

#define INITFS_VERSION 1

struct initfs_header {
    /// The entry point of the memmgr.
    uint8_t jump_code[16];
    /// THe initfs format version. Must be `1'.
    uint32_t version;
    /// The size of the file system excluding this header.
    uint32_t total_size;
    /// The number of files in the file system.
    uint32_t num_files;
    /// Rerserved for future use (currently unused).
    uint8_t reserved[100];
} PACKED;

STATIC_ASSERT(sizeof(struct initfs_header) == 128);

struct initfs_file {
    /// The file path (null-terminated).
    char path[48];
    /// The size of files.
    uint32_t len;
    /// The size of the padding space immediately after the file content.
    /// Padding will be added to align the beginning of the files with
    /// the page size.
    uint16_t padding_len;
    /// Rerserved for future use (currently unused).
    uint8_t reserved[10];
    /// File content (it comes right after the this header). Note that this
    /// it a zero-length array.
    uint8_t content[];
} PACKED;

STATIC_ASSERT(sizeof(struct initfs_file) == 64);

struct initfs_dir {
    uint32_t index;
    struct initfs_file *next;
};

void initfs_opendir(struct initfs_dir *dir);
const struct initfs_file *initfs_readdir(struct initfs_dir *dir);
void initfs_init(void);

#endif
