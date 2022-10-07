#include "bootfs.h"
#include <string.h>

extern char __bootfs[];
static struct bootfs_file *files;
static unsigned num_files;

void bootfs_read(struct bootfs_file *file, offset_t off, void *buf,
                 size_t len) {
    void *p = (void *) (((uintptr_t) __bootfs) + file->offset + off);
    memcpy(buf, p, len);
}

struct bootfs_file *bootfs_open(const char *path) {
    struct bootfs_file *file;
    for (int i = 0; (file = bootfs_open_iter(i)) != NULL; i++) {
        if (!strncmp(file->name, path, sizeof(file->name))) {
            return file;
        }
    }

    return NULL;
}

struct bootfs_file *bootfs_open_iter(unsigned index) {
    if (index >= num_files) {
        return NULL;
    }

    return &files[index];
}

void bootfs_init(void) {
    struct bootfs_header *header = (struct bootfs_header *) __bootfs;
    num_files = header->num_files;
    files =
        (struct bootfs_file *) (((uintptr_t) &__bootfs) + header->files_off);
}
