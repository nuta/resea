#include "initfs.h"

#include <resea.h>
#include <resea/string.h>

extern const struct initfs_header __initfs;

void initfs_opendir(struct initfs_dir *dir) {
    dir->index = 0;
    dir->next =
        (struct initfs_file *) ((uintptr_t) &__initfs + sizeof(__initfs));
}

const struct initfs_file *initfs_readdir(struct initfs_dir *dir) {
    if (dir->index >= __initfs.num_files) {
        return NULL;
    }

    struct initfs_file *current = dir->next;
    dir->next = (struct initfs_file *) ((uintptr_t) current +
                                        sizeof(struct initfs_file) +
                                        current->len + current->padding_len);
    dir->index++;
    return current;
}

void initfs_init(void) {
    if (__initfs.version != INITFS_VERSION) {
        ERROR("invalid initfs version: %d", __initfs.version);
    }

    TRACE("initfs: num_of_files=%d", __initfs.num_files);
}
