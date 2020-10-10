#ifndef __FS_H__
#define __FS_H__

#include <types.h>

void fs_read(const char *path);
void fs_write(const char *path, const uint8_t *buf, size_t len);

#endif
