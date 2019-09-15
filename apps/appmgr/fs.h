#ifndef __FS_H__
#define __FS_H__

#include <types.h>

#define PATH_MAX 128

struct file {
    bool using;
    cid_t fs_server;
    fd_t fd;
};

#endif
