#ifndef __WEBAPI_H__
#define __WEBAPI_H__

#include <string.h>
#include <list.h>
#include <types.h>

struct client {
    list_elem_t next;
    handle_t handle;
    bool done;
    char *request;
    size_t request_len;
};

#endif
