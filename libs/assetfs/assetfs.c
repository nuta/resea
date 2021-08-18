#include <assetfs.h>
#include <string.h>

error_t assetfs_open(struct assetfs *fs, void *image, size_t image_size) {
    struct assetfs_header *header = (struct assetfs_header *) image;
    if (header->magic != ASSETFS_MAGIC) {
        return ERR_INVALID_ARG;
    }

    fs->image = image;
    fs->num_entries = header->num_entries;
    fs->entries = header->entries;
    return OK;
}

struct assetfs_file *assetfs_open_file(struct assetfs *fs, const char *name) {
    size_t name_len = strlen(name);
    if (name_len >= 48) {
        return NULL;
    }

    for (uint32_t i = 0; i < fs->num_entries; i++) {
        struct assetfs_file *file = &fs->entries[i];
        if (!strncmp(file->name, name, name_len)) {
            return file;
        }
    }

    return NULL;
}

void *assetfs_file_data(struct assetfs *fs, struct assetfs_file *file) {
    return fs->image + file->offset;
}
