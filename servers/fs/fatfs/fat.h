#ifndef __FAT_H__
#define __FAT_H__

#include <types.h>

typedef uint32_t cluster_t;
#define SECTOR_SIZE 512

enum fat_type {
    FAT16,
};

struct fat {
    enum fat_type type;
    size_t sectors_per_cluster;
    /// The FAT table.
    offset_t fat_lba;
    size_t sectors_per_fat;
    /// The root directory entries (FAT12/16).
    cluster_t root_dir_lba;
    offset_t data_lba;

    void (*blk_read)(offset_t sector, void *buf, size_t num_sectors);
    void (*blk_write)(offset_t sector, const void *buf, size_t num_sectors);
};

struct fat_file {
    /// The beginning of data.
    cluster_t cluster;
    /// The size of the file in bytes.
    size_t size;
};

struct fat_dirent;
struct fat_dir {
    /// Entries in the current cluster.
    struct fat_dirent *entries;
    /// The current cluster.
    cluster_t cluster;
    /// The next entry index in `entires`. -1 if there's no next entry.
    int index;
};

//
//  On-disk data structures.
//

struct bpb {
    uint8_t   jmp[3];
    uint8_t   oem_name[8];
    uint16_t  sector_size;
    uint8_t   sectors_per_cluster;
    uint16_t  num_reserved_sectors;
    uint8_t   num_fat;
    uint16_t  num_root_entries;
    uint16_t  num_total_sectors16;
    uint8_t   media_id;
    uint16_t  sectors_per_fat16;
    uint16_t  sectors_per_track;
    uint16_t  num_head;
    uint32_t  hidden_sectors;
    uint32_t  num_total_sectors32;
    uint32_t  sectors_per_fat32;
    uint16_t  flags;
    uint16_t  fat_version;
    uint32_t  root_entries_cluster;
    uint16_t  fsinfo_sector;
    uint16_t  sectors_per_backup_boot;
    uint8_t   reserved[12];
    uint8_t   drive_number;
    uint8_t   winnt_flags;
    uint8_t   signature;
    uint8_t   volume_id[4];
    uint8_t   volume_label[11];
    uint8_t   fat32_string[8];
    uint8_t   bootcode[420];
    uint8_t   magic[2]; // 0x55, 0xaa
} __packed;

struct fat_dirent {
    // name[0]
    // 0xe5 / 0x00: unused
    // 0x05: replace with 0xe5
    uint8_t   name[8];
    uint8_t   ext[3];
    uint8_t   attr;
    uint8_t   winnt;
    uint8_t   time_created_tenth;
    uint16_t  time_created;
    uint16_t  date_created;
    uint16_t  date_accessed;
    uint16_t  cluster_begin_high;
    uint16_t  time_modified;
    uint16_t  date_modified;
    uint16_t  cluster_begin_low;
    uint32_t  size;
} __packed;

error_t fat_probe(struct fat *fs,
                  void (*blk_read)(offset_t sector, void *buf, size_t num_sectors),
                  void (*blk_write)(offset_t sector, const void *buf, size_t num_sectors));
error_t fat_open(struct fat *fs, struct fat_file *file, const char *path);
error_t fat_create(struct fat *fs, struct fat_file *file, const char *path, bool exist_ok);
int fat_read(struct fat *fs, struct fat_file *file, offset_t off, void *buf,
             size_t len);
int fat_write(struct fat *fs, struct fat_file *file, offset_t off,
              const void *buf, size_t len);
error_t fat_opendir(struct fat *fs, struct fat_dir *dir, const char *path);
void fat_closedir(struct fat *fs, struct fat_dir *dir);
struct fat_dirent *fat_readdir(struct fat *fs, struct fat_dir *dir);

#endif
