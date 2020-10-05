#include "fat.h"
#include <resea/printf.h>
#include <resea/malloc.h>
#include <string.h>
#include <resea/ctype.h>

error_t fat_probe(struct fat *fs,
                  void (*blk_read)(size_t offset, void *buf, size_t len),
                  void (*blk_write)(size_t offset, const void *buf, size_t len)) {
    struct bpb bpb;
    STATIC_ASSERT(sizeof(bpb) == SECTOR_SIZE);
    blk_read(0, &bpb, 1);

    if (bpb.sector_size != SECTOR_SIZE) {
        WARN("unexpected sector size: %d (expected to be %d)",
             bpb.sector_size, SECTOR_SIZE);
        return ERR_NOT_ACCEPTABLE;
    }

    // TODO: Check if the disk is a valid FAT file system.

    // get the number of clusters
    size_t root_dir_sectors = (bpb.num_root_entries * 32) / bpb.sector_size;

    size_t sectors_per_fat;
    if (bpb.sectors_per_fat16 == 0) {
      sectors_per_fat = bpb.sectors_per_fat32;
    } else {
      sectors_per_fat = bpb.sectors_per_fat16;
    }

    size_t sectors;
    if (bpb.num_total_sectors16 == 0) {
      sectors = bpb.num_total_sectors32;
    } else {
      sectors = bpb.num_total_sectors16;
    }

    size_t non_data_sectors =
        bpb.num_reserved_sectors + (bpb.num_fat * sectors_per_fat)
        + root_dir_sectors;
    size_t total_data_clus =
        (sectors - non_data_sectors) / bpb.sectors_per_cluster;

    // Determine the type of the file system: FAT12, FAT16, or FAT32.
    enum fat_type type;
    if (total_data_clus < 4085) {
        // FAT12 is not supported for now.
        return ERR_NOT_ACCEPTABLE;
    } else if (total_data_clus < 65525) {
        type = FAT16;
        fs->sectors_per_fat = bpb.sectors_per_fat16;
    } else {
        // FAT32 is not supported for now.
        return ERR_NOT_ACCEPTABLE;
    }

    fs->type = type;
    fs->blk_read = blk_read;
    fs->blk_write = blk_write;
    fs->sectors_per_cluster = bpb.sectors_per_cluster;
    fs->fat_lba = bpb.num_reserved_sectors;
    fs->root_dir_lba = fs->fat_lba + sectors_per_fat * bpb.num_fat;
    fs->data_lba = fs->root_dir_lba + root_dir_sectors;
    return OK;
}

/// Returns true if it's the end of cluster.
static bool is_end_of_cluster(struct fat *fs, cluster_t cluster) {
    switch (fs->type) {
        case FAT16: return 0xfff8 <= cluster && cluster <= 0xffff;
    }
}

static bool is_free_cluster(cluster_t cluster) {
    return cluster == 0;
}

static bool is_valid_cluster(struct fat *fs, cluster_t cluster) {
    return !is_end_of_cluster(fs, cluster) && !is_free_cluster(cluster);
}

static bool filename_equals(struct fat_dirent *e, const char *name,
                            const char *ext) {
    int name_len = 8;
    while (name_len >= 0 && e->name[name_len - 1] == ' ') {
        name_len--;
    }

    int ext_len = 3;
    while (ext_len > 0 && e->ext[ext_len - 1] == ' ') {
        ext_len--;
    }

    return !strncmp((const char *) e->name, name, name_len)
        && !strncmp((const char *) e->ext, ext, ext_len);
}

static char *get_next_filename(char *path, char *name, char *ext) {
    int i;
    for (i = 0; i < 8 && *path; i++) {
        if (*path == '/') {
            break;
        }else if (*path == '.') {
            path++;
            break;
        }

        name[i] = toupper(*path);
        path++;
    }
    name[i] = '\0';

    for (i = 0; i < 3 && *path; i++) {
        if (*path == '/') {
            path++;
            break;
        }

        ext[i] = toupper(*path);
        path++;
    }

    ext[i] = '\0';
    return (*path == '\0') ? NULL : path;
}

static cluster_t get_cluster_from_entry(struct fat_dirent *e) {
    return ((e->cluster_begin_high << 16) | e->cluster_begin_low);
}

static offset_t cluster2lba(struct fat *fs, cluster_t cluster) {
    DEBUG_ASSERT(cluster >= 2);
    return (((cluster - 2) * fs->sectors_per_cluster) + fs->data_lba);
}

static offset_t get_next_cluster(struct fat *fs, cluster_t cluster) {
    DEBUG_ASSERT(cluster >= 2);
    size_t fat_ent_size, entries_per_sector;
    switch (fs->type) {
        case FAT16:
            fat_ent_size = 2;
            entries_per_sector = SECTOR_SIZE / sizeof(uint16_t);
            break;
    }

    uint8_t buf[SECTOR_SIZE];
    fs->blk_read(fs->fat_lba + (cluster / entries_per_sector), &buf, 1);

    switch (fs->type) {
        case FAT16: {
            return ((uint16_t *) buf)[cluster % entries_per_sector];
        }
    }
}

static offset_t alloc_cluster(struct fat *fs) {
    size_t fat_ent_size, entries_per_sector;
    switch (fs->type) {
        case FAT16:
            fat_ent_size = 2;
            entries_per_sector = SECTOR_SIZE / sizeof(uint16_t);
            break;
    }

    // Look for an unused cluster in the FAT table.
    for (size_t i = 0; i < fs->sectors_per_fat; i++) {
        __aligned(4) uint8_t buf[SECTOR_SIZE];

        fs->blk_read(fs->fat_lba + i, &buf, 1);
        switch (fs->type) {
            case FAT16: {
                uint16_t *table = (uint16_t *) buf;
                size_t entries_per_sector = sizeof(buf) / sizeof(*table);
                for (size_t j = 0; j < entries_per_sector; j++) {
                    if (*table == 0) {
                        DBG("alloc = %d", i * entries_per_sector + j);
                        return i * entries_per_sector + j;
                    }
                }
                break;
            }
        }
    }

    // TODO: Return an error instead.
    PANIC("run out of free clusters");
}

static void open_root_dir(struct fat *fs, struct fat_dir *dir) {
    offset_t lba;
    switch (fs->type) {
        case FAT16:
            // In FAT16, root directory entries is located in front of data
            // clusters.
            dir->cluster = 0;
            lba = fs->root_dir_lba;
            break;
    }

    dir->entries = malloc(fs->sectors_per_cluster * SECTOR_SIZE);
    dir->index = 0;
    fs->blk_read(lba, dir->entries, fs->sectors_per_cluster);
}

static void opendir_from_dirent(struct fat *fs, struct fat_dir *dir,
                             struct fat_dirent *e) {
    dir->cluster = get_cluster_from_entry(e);
    dir->entries = malloc(fs->sectors_per_cluster * SECTOR_SIZE);
    dir->index = 0;
    fs->blk_read(cluster2lba(fs, dir->cluster), dir->entries,
                 fs->sectors_per_cluster);
}

/// Looks for the file from the root directory.
static struct fat_dirent *lookup(struct fat *fs, const char *path) {
    char *p = (char *) path;
    if (*p == '/') {
        p++;
    }

    struct fat_dir dir;
    open_root_dir(fs, &dir);
    while(1) {
        char name[9];
        char ext[4];
        p = get_next_filename(p, (char *) name, (char *) ext);
        while (1) {
            struct fat_dirent *e = fat_readdir(fs, &dir);
            if (!e) {
                // No such a file.
                return NULL;
            }

            if (filename_equals(e, (const char *) name, (const char *) ext)) {
                if (!p) {
                    // Found the file!
                    return e;
                }

                // Enter the next directory level.
                opendir_from_dirent(fs, &dir, e);
                break;
            }
        }
    }
}

error_t fat_open(struct fat *fs, struct fat_file *file, const char *path) {
    struct fat_dirent *e = lookup(fs, path);
    if (!e) {
        return ERR_NOT_FOUND;
    }

    file->cluster = get_cluster_from_entry(e);
    file->size = e->size;
    return OK;
}

error_t fat_truncate(struct fat *fs, struct fat_file *file, offset_t offset) {
    // TODO:
    OOPS("NYI");
    return OK;
}

error_t fat_create(struct fat *fs, struct fat_file *file, const char *path, bool exist_ok) {
    if (fat_open(fs, file, path) == OK) {
        // The file already exists.
        if (!exist_ok) {
            return ERR_ALREADY_EXISTS;
        }

        return fat_truncate(fs, file, 0);
    }

    // The file does not exist. Create a new dir entry.
    NYI();
    return OK;
}

int fat_read(struct fat *fs, struct fat_file *file, offset_t off, void *buf,
             size_t len) {
    if (off + len < file->size && off + len < off) {
        return ERR_TOO_LARGE;
    }

    // Traverse the FAT table until the target cluster.
    cluster_t current = file->cluster;
    size_t nth_cluster = off / (fs->sectors_per_cluster * SECTOR_SIZE);
    while (nth_cluster > 0) {
        current = get_next_cluster(fs, current);
        if (is_end_of_cluster(fs, current)) {
            return 0;
        }

        ASSERT(is_valid_cluster(fs, current));
        nth_cluster--;
    }

    offset_t off_in_cluster = off % (fs->sectors_per_cluster * SECTOR_SIZE);
    uint8_t *p = buf;
    size_t remaining = len;
    while (true) {
        offset_t sector_offset = off_in_cluster / SECTOR_SIZE;
        for (offset_t i = sector_offset; i < fs->sectors_per_cluster; i++) {
            // Use a temporary buffer to support unaligned read operations.
            uint8_t buf[SECTOR_SIZE];
            fs->blk_read(cluster2lba(fs, current) + i, buf, 1);
            size_t copy_len = MIN(file->size, MIN(remaining, SECTOR_SIZE));
            memcpy(p, &buf[off_in_cluster], copy_len);

            if (remaining <= SECTOR_SIZE) {
                return copy_len;
            }

            p += copy_len;
            remaining -= copy_len;
        }

        off_in_cluster = 0;
        current = get_next_cluster(fs, current);
        if (is_end_of_cluster(fs, current)) {
            return len - remaining;
        }

        ASSERT(is_valid_cluster(fs, current));
    }
}

int fat_write(struct fat *fs, struct fat_file *file, offset_t off,
              const void *buf, size_t len) {
    if (off + len < file->size && off + len < off) {
        return ERR_TOO_LARGE;
    }

    // Traverse the FAT table until the target cluster.
    cluster_t current = file->cluster;
    size_t nth_cluster = off / (fs->sectors_per_cluster * SECTOR_SIZE);
    while (nth_cluster > 0) {
        current = get_next_cluster(fs, current);
        if (is_end_of_cluster(fs, current)) {
            current = alloc_cluster(fs);
        }

        ASSERT(is_valid_cluster(fs, current));
        nth_cluster--;
    }

    offset_t off_in_cluster = off % (fs->sectors_per_cluster * SECTOR_SIZE);
    const uint8_t *p = buf;
    size_t remaining = len;
    while (true) {
        offset_t sector_offset = off_in_cluster / SECTOR_SIZE;
        for (offset_t i = sector_offset; i < fs->sectors_per_cluster; i++) {
            // Use a temporary buffer to support unaligned read operations.
            size_t copy_len = MIN(file->size, MIN(remaining, SECTOR_SIZE));
            uint8_t buf[SECTOR_SIZE];
            fs->blk_read(cluster2lba(fs, current) + i, buf, 1);
            memcpy(&buf[off_in_cluster], p, copy_len);
            fs->blk_write(cluster2lba(fs, current) + i, buf, 1);

            if (remaining <= SECTOR_SIZE) {
                return copy_len;
            }

            p += copy_len;
            remaining -= copy_len;
        }

        off_in_cluster = 0;
        current = get_next_cluster(fs, current);
        if (is_end_of_cluster(fs, current)) {
            current = alloc_cluster(fs);
        }

        ASSERT(is_valid_cluster(fs, current));
    }
}

error_t fat_opendir(struct fat *fs, struct fat_dir *dir, const char *path) {
    if (!strcmp(path, "/")) {
        open_root_dir(fs, dir);
        return OK;
    }

    struct fat_dirent *e = lookup(fs, path);
    if (!e) {
        return ERR_NOT_FOUND;
    }

    opendir_from_dirent(fs, dir, e);
    return OK;
}

void fat_closedir(struct fat *fs, struct fat_dir *dir) {
    free(dir->entries);
}


struct fat_dirent *fat_readdir(struct fat *fs, struct fat_dir *dir) {
    if (dir->index < 0) {
        return NULL;
    }

    struct fat_dirent *e = &dir->entries[dir->index];
    if (!e->name[0]) {
        dir->index = -1;
        return NULL;
    }

    dir->index++;
    int num_entries =
        (fs->sectors_per_cluster * SECTOR_SIZE) / sizeof(struct fat_dirent);
    if (dir->index == num_entries) {
        // Read the next cluster.
        dir->cluster = get_next_cluster(fs, dir->cluster);
        if (dir->cluster) {
            dir->index = 0;
            fs->blk_read(cluster2lba(fs, dir->cluster), dir->entries,
                         fs->sectors_per_cluster);
        } else {
            dir->index = -1;
        }
    }

    return e;
}
