#ifndef __KVS_H__
#define __KVS_H__

#include <list.h>
#include <message.h>

struct entry {
    list_elem_t next;
    char key[KVS_KEY_LEN];
    size_t len;
    list_t listeners;
    uint8_t data[KVS_DATA_LEN_MAX];
};

struct listener {
    list_elem_t next;
    tid_t task;
    bool changed;
};

#endif
