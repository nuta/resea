#ifndef __WEBAPI_H__
#define __WEBAPI_H__

#include <string.h>
#include <types.h>

struct client {
    handle_t handle;
    bool done;
    char *request;
    size_t request_len;
};

#endif
