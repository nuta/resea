#ifndef __WEBAPI_H__
#define __WEBAPI_H__

#include <list.h>
#include <string.h>
#include <types.h>

struct client {
    list_elem_t next;
    handle_t handle;
    bool done;
    char *request;
    size_t request_len;
};

#endif
